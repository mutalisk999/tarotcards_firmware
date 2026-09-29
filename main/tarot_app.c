// main/tarot_app.c —— 塔罗应用主任务:按键队列、页面切换、抽牌数据准备。
// 运行模型沿用仓库基线:按键回调只入队;本任务在 bsp_lvgl_lock 下操作 LVGL。
#include "tarot_app.h"

#include <string.h>

#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "tarot_app_internal.h"
#include "tarot_theme.h"

static const char *TAG = "tarot";

#define TAROT_INPUT_QUEUE_DEPTH 8
#define TAROT_BATTERY_POLL_MS 6000
#define TAROT_TASK_STACK_BYTES 6144
#define TAROT_TASK_PRIORITY 5

typedef struct {
    bsp_btn_t btn;
    bsp_btn_ev_t ev;
} tarot_input_event_t;

// 队列唤醒标记:定时器回调发起切页后叫醒应用任务(事件本身无按键语义)。
// 取 -1 是因为 bsp_btn_ev_t 的合法取值是 0..3,-1 永不会与真实按键事件撞车。
#define TAROT_EV_WAKE ((bsp_btn_ev_t)-1)

static struct tarot_app_s s_app;
static tarot_page_t s_next_state;
static QueueHandle_t s_queue;

// 单一入口查找某状态的页面操作表;三组 provider 各自只认自己的状态。
static const tarot_page_ops_t *page_ops(tarot_page_t state) {
    if ((int)state < 0 || (int)state >= TAROT_PAGE_COUNT) return NULL;
    const tarot_page_ops_t *ops = tarot_page_ops_home(state);
    if (!ops) ops = tarot_page_ops_cast(state);
    if (!ops) ops = tarot_page_ops_card(state);
    return ops;
}

// 编译期保证:页面状态数与三组 provider 覆盖一致。
_Static_assert(TAROT_PAGE_COUNT == 3, "新增页面状态必须同时在三组 provider 中登记");

void tarot_style_option(lv_obj_t *panel, bool selected) {
    // 选中:宣纸底 + 朱砂字与描边 + 投影浮起;未选中:墨底面板 + 宣纸字。
    if (selected) {
        lv_obj_set_style_bg_color(panel, lv_color_hex(TT_COLOR_PAPER), 0);
        lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(panel, lv_color_hex(TT_COLOR_CINNABAR), 0);
        lv_obj_set_style_border_width(panel, 2, 0);
        lv_obj_set_style_border_color(panel, lv_color_hex(TT_COLOR_CINNABAR), 0);
        lv_obj_set_style_radius(panel, 8, 0);
        lv_obj_set_style_shadow_color(panel, lv_color_hex(TT_COLOR_CINNABAR), 0);
        lv_obj_set_style_shadow_opa(panel, LV_OPA_40, 0);
        lv_obj_set_style_shadow_width(panel, 10, 0);
        lv_obj_set_style_shadow_spread(panel, 1, 0);
    } else {
        lv_obj_set_style_bg_color(panel, lv_color_hex(TT_COLOR_PANEL), 0);
        lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(panel, lv_color_hex(TT_COLOR_PAPER), 0);
        lv_obj_set_style_border_width(panel, 1, 0);
        lv_obj_set_style_border_color(panel, lv_color_hex(TT_COLOR_PANEL_2), 0);
        lv_obj_set_style_radius(panel, 8, 0);
        lv_obj_set_style_shadow_width(panel, 0, 0);
        lv_obj_set_style_shadow_opa(panel, LV_OPA_TRANSP, 0);
    }
}

static void refresh_battery(struct tarot_app_s *app) {
    if (app->battery) {
        tarot_battery_update(app->battery, bsp_battery_soc());
    }
}

// 切页请求:仅置位,实际删屏重建在本轮锁内完成(见 process_event)。
void tarot_app_goto(struct tarot_app_s *app, tarot_page_t next) {
    s_next_state = next;
    app->switch_requested = true;
}

static void enter_state(struct tarot_app_s *app, tarot_page_t state) {
    app->state = state;
    app->screen = NULL;
    app->battery = NULL;
    // 上一次页面的控件句柄随屏幕删除已失效:全部清空,避免悬空指针。
    memset(&app->home, 0, sizeof(app->home));
    memset(&app->cast, 0, sizeof(app->cast));
    memset(&app->card, 0, sizeof(app->card));
    const tarot_page_ops_t *ops = page_ops(state);
    if (ops && ops->build) {
        ops->build(app);
    }
    if (!app->screen) {
        // 某状态没有 ops 表项,或 build 因内存不足未创建出屏幕:
        // 明确告警并退回封面,而不是留一张空屏让后续按键操作悬空指针。
        ESP_LOGE(TAG, "状态 %d 无可用页面或页面构建失败,退回封面", (int)state);
        ops = page_ops(TAROT_PAGE_HOME);
        if (ops && ops->build) ops->build(app);
        app->state = app->screen ? TAROT_PAGE_HOME : app->state;
    }
    // 规范默认位:页面右上角电量。
    if (app->screen) {
        app->battery = lv_label_create(app->screen);
        if (app->battery) {
            lv_obj_set_style_text_font(app->battery, &tarot_font_16, 0);
            lv_obj_set_style_text_color(app->battery,
                                        lv_color_hex(TT_COLOR_PAPER_DIM), 0);
            lv_obj_set_pos(app->battery, 182, 10);
        }
        refresh_battery(app);
        // 不做整屏淡入:单缓冲部分刷新下,透明度接近 0 的首帧会把缓冲区
        // 里的陈旧内容透出来,表现为切页闪屏;本板无 PSRAM,也无法用
        // 离屏层做正确混合。瞬时切页是最稳的表现。
        lv_screen_load(app->screen);
    }
}

static void apply_switch(struct tarot_app_s *app) {
    tarot_page_t next = s_next_state;
    app->switch_requested = false;
    const tarot_page_ops_t *ops = page_ops(app->state);
    if (ops && ops->exit) {
        ops->exit(app);
    }
    if (app->screen) {
        lv_obj_delete(app->screen);
        app->screen = NULL;
        app->battery = NULL;
    }
    enter_state(app, next);
}

static void process_event(struct tarot_app_s *app,
                          const tarot_input_event_t *event) {
    if (event->ev == TAROT_EV_WAKE) {
        if (app->switch_requested) {
            apply_switch(app);
        }
        return;
    }
    if (app->switch_requested) {
        // 上一次切页尚未执行(例如 WAKE 在队列满时被丢弃)。
        // 直接在此补做,而不是丢弃本次按键。
        apply_switch(app);
        if (app->switch_requested) return;  // 补做仍失败,放弃本事件
    }
    const tarot_page_ops_t *ops = page_ops(app->state);
    if (ops && ops->key) {
        ops->key(app, event->btn, event->ev);
    }
    if (app->switch_requested) {
        apply_switch(app);
    }
}

void tarot_app_notify(struct tarot_app_s *app) {
    if (app->queue) {
        const tarot_input_event_t wake = {.btn = BSP_BTN_UP, .ev = TAROT_EV_WAKE};
        (void)xQueueSend(app->queue, &wake, 0);
    }
}

static void app_task(void *arg) {
    struct tarot_app_s *app = (struct tarot_app_s *)arg;
    tarot_input_event_t event;
    int last_soc = -2;  // -2 表示尚未渲染过,与"读失败 -1"区分
    for (;;) {
        if (xQueueReceive(app->queue, &event,
                          pdMS_TO_TICKS(TAROT_BATTERY_POLL_MS)) == pdTRUE) {
            // LVGL 非线程安全:页面重建/控件更新一律持锁。
            // 拿不到锁说明 LVGL 任务没有让出,此时事件会被丢弃;
            // 连续失败时明确告警,避免问题表现为"按键毫无反应"而无任何日志。
            static int lock_stall;
            if (bsp_lvgl_lock(500)) {
                lock_stall = 0;
                process_event(app, &event);
                bsp_lvgl_unlock();
            } else if (lock_stall++ == 2) {
                ESP_LOGE(TAG, "LVGL 锁持续不可用,输入事件已开始丢弃");
            }
        } else {
            // 定时轮询分支:补做可能丢失的切页请求(WAKE 丢包兜底)。
            if (app->switch_requested) {
                if (bsp_lvgl_lock(500)) {
                    apply_switch(app);
                    bsp_lvgl_unlock();
                }
                continue;
            }
            // SOC 读取先在锁外做(I2C 时延不阻塞渲染),再持锁刷新标签。
            int soc = bsp_battery_soc();
            if (soc == last_soc) continue;
            last_soc = soc;
            if (bsp_lvgl_lock(200)) {
                refresh_battery(app);
                bsp_lvgl_unlock();
            }
        }
    }
}

// 按键回调运行在共享 esp_timer 任务:只入队,立即返回。
static void on_key(bsp_btn_t btn, bsp_btn_ev_t ev, void *user) {
    (void)user;
    const tarot_input_event_t event = {.btn = btn, .ev = ev};
    (void)xQueueSend(s_queue, &event, 0);
}

void tarot_app_start(void) {
    memset(&s_app, 0, sizeof(s_app));

    s_queue = xQueueCreate(TAROT_INPUT_QUEUE_DEPTH, sizeof(tarot_input_event_t));
    // 先赋值再建任务:应用任务优先级更高,创建后立即调度,
    // 不能让它看到尚未赋值的 queue。
    s_app.queue = s_queue;
    if (!s_queue || xTaskCreate(app_task, "tarot_app", TAROT_TASK_STACK_BYTES,
                                &s_app, TAROT_TASK_PRIORITY, NULL) != pdPASS) {
        ESP_LOGE(TAG, "应用任务创建失败");
        return;
    }

    if (bsp_button_init(on_key, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "按键初始化失败,应用无法交互");
        return;
    }

    if (bsp_lvgl_lock(1000)) {
        enter_state(&s_app, TAROT_PAGE_HOME);
        bsp_lvgl_unlock();
    } else {
        ESP_LOGE(TAG, "LVGL 锁获取失败");
    }
    ESP_LOGI(TAG, "塔罗应用就绪");
}
