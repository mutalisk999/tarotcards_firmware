// main/tarot_app_internal.h —— 塔罗应用内部共享结构(仅 tarot_app*.c 与页面文件使用)。
#pragma once

#include "bsp_button.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "lvgl.h"
#include "tarot_engine.h"
#include "tarot_nav.h"

// 洗牌页部件:卡位背面 + 位置标签 + 结果标签。
typedef struct {
    lv_obj_t *slots[TAROT_DRAW_MAX];   // 卡位容器(牌背画在里面)
    lv_obj_t *backs[TAROT_DRAW_MAX];   // 牌背矩形(动画抖动对象)
    lv_obj_t *slot_labels[TAROT_DRAW_MAX];  // 过去/现在/未来(单张为"此刻")
    lv_obj_t *result_label;            // 最近一次翻开的"牌名 · 正/逆"
    lv_obj_t *hint_label;
} tarot_cast_widgets_t;

// 首页部件:两个牌阵选项面板。
typedef struct {
    lv_obj_t *options[TAROT_MODE_COUNT];
} tarot_home_widgets_t;

// 牌面页部件。
typedef struct {
    lv_obj_t *tabs[TAROT_DRAW_MAX];    // 过去/现在/未来位置页签(单张隐藏)
    lv_obj_t *emblem;                  // 牌面符号容器(每次翻牌重建内容)
    lv_obj_t *name_label;              // 牌名(24px)
    lv_obj_t *orient_label;            // 正位/逆位 徽记
    lv_obj_t *kw_label;                // 关键词
    lv_obj_t *scroll;                  // 解读滚动容器
    lv_obj_t *body;                    // 解读文本
} tarot_card_widgets_t;

struct tarot_app_s {
    QueueHandle_t queue;
    lv_obj_t *screen;
    lv_obj_t *battery;
    tarot_page_t state;
    bool switch_requested;   // 本轮输入已请求切页
    int sel;                 // 首页选中的牌阵(下标)
    tarot_mode_t mode;       // 本次占卜的牌阵
    int draw_count;          // 本次占卜的张数(1 或 3)
    tarot_spread_t spread;   // 一次洗牌产出的完整牌阵
    bool spread_valid;
    int revealed;            // 已翻开的牌数(洗牌页)
    bool cast_busy;          // 洗牌动画进行中
    lv_timer_t *anim_timer;  // 洗牌动画定时器(切页前必须删)
    lv_timer_t *auto_timer;  // 单张抽完自动进牌面(同上)
    int card_index;          // 牌面页当前查看的牌
    // 页面部件。
    tarot_home_widgets_t home;
    tarot_cast_widgets_t cast;
    tarot_card_widgets_t card;
};

// 页面操作表:build/key 均在持有 bsp_lvgl_lock 时调用;
// exit 在删除屏幕前调用(清理定时器等)。
typedef struct {
    void (*build)(struct tarot_app_s *app);
    void (*key)(struct tarot_app_s *app, bsp_btn_t btn, bsp_btn_ev_t ev);
    void (*exit)(struct tarot_app_s *app);
} tarot_page_ops_t;

// 三组页面的操作表(tarot_pages_home/cast/card.c 各实现其一)。
const tarot_page_ops_t *tarot_page_ops_home(tarot_page_t state);
const tarot_page_ops_t *tarot_page_ops_cast(tarot_page_t state);
const tarot_page_ops_t *tarot_page_ops_card(tarot_page_t state);

// LVGL 任务侧(定时器回调)通知应用任务立即处理切页请求。
void tarot_app_notify(struct tarot_app_s *app);

// —— 供各页面共用的工具(tarot_app.c 提供) ——
void tarot_app_goto(struct tarot_app_s *app, tarot_page_t next);  // 请求切页

// 页面内小组件:选项面板选中态(宣纸底 + 朱砂描边)。
void tarot_style_option(lv_obj_t *panel, bool selected);
