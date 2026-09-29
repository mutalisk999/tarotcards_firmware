// main/tarot_pages_cast.c —— 洗牌占卜页:动画洗牌,逐张翻开。
// 一次洗牌产出完整牌阵(引擎一次抽取),用户逐张按 OK 翻开;
// 单张模式翻完自动进入牌面页,三牌阵由用户节奏控制。
#include <stdio.h>
#include <string.h>

#include "bsp_button.h"
#include "esp_log.h"
#include "esp_random.h"
#include "tarot_app_internal.h"
#include "tarot_art.h"
#include "tarot_deck.h"
#include "tarot_emblem.h"
#include "tarot_theme.h"

#define CAST_ANIM_TICKS 9
#define CAST_ANIM_PERIOD_MS 70
#define CAST_AUTO_NEXT_MS 900

static const char *TAG = "tarot-cast";

// 动画帧计数与所属应用(同一时刻至多一段洗牌动画,文件内静态即可;
// LVGL 9 的 lv_timer_t 不透明,不能直接读字段)。
static int s_cast_ticks;
static struct tarot_app_s *s_cast_app;

static void cast_auto_next(lv_timer_t *timer);
static bool slot_label_ok(struct tarot_app_s *app);

// 牌背:金边圆角矩形 + 中心圆点。挂在 slot 容器里,动画只抖动它。
static void cast_make_back(struct tarot_app_s *app, int slot) {
    lv_obj_t *back = tarot_rect_create(app->cast.slots[slot], 4, 4,
                                       lv_obj_get_width(app->cast.slots[slot]) - 8,
                                       lv_obj_get_height(app->cast.slots[slot]) - 8,
                                       TT_COLOR_PANEL_2);
    lv_obj_set_style_border_width(back, 2, 0);
    lv_obj_set_style_border_color(back, lv_color_hex(TT_COLOR_GOLD_DIM), 0);
    lv_obj_set_style_radius(back, 8, 0);
    lv_obj_t *eye = lv_obj_create(back);
    lv_obj_remove_style_all(eye);
    int w = lv_obj_get_width(back);
    int h = lv_obj_get_height(back);
    lv_obj_set_size(eye, w / 4, w / 4);
    lv_obj_align(eye, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(eye, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(eye, lv_color_hex(TT_COLOR_GOLD_DIM), 0);
    lv_obj_set_style_bg_opa(eye, LV_OPA_60, 0);
    app->cast.backs[slot] = back;
}

static void cast_build(struct tarot_app_s *app) {
    app->screen = tarot_page_create("洗牌占卜");
    app->cast_busy = false;
    app->anim_timer = NULL;
    app->auto_timer = NULL;
    s_cast_app = app;
    if (app->draw_count != TAROT_SPREAD_SINGLE &&
        app->draw_count != TAROT_SPREAD_THREE) {
        // 防御:非法张数按单张处理,避免越界写 slots。
        app->draw_count = TAROT_SPREAD_SINGLE;
    }

    if (app->draw_count == TAROT_SPREAD_THREE) {
        // 三张牌位:64x96,间隔 12,总宽 216 居中。
        static const char *const k_positions[3] = {"过去", "现在", "未来"};
        for (int i = 0; i < TAROT_SPREAD_THREE; i++) {
            lv_obj_t *slot = lv_obj_create(app->screen);
            lv_obj_remove_style_all(slot);
            lv_obj_set_pos(slot, 12 + i * 76, 56);
            lv_obj_set_size(slot, 64, 96);
            app->cast.slots[i] = slot;
            cast_make_back(app, i);

            lv_obj_t *label = lv_label_create(app->screen);
            lv_obj_set_style_text_font(label, &tarot_font_16, 0);
            lv_obj_set_style_text_color(label,
                                        lv_color_hex(TT_COLOR_PAPER_DIM), 0);
            lv_label_set_text(label, k_positions[i]);
            // 只用绝对定位:若再叠加 align(TOP_MID),set_pos 会变成
            // 对齐基准上的偏移,整排标签被推向右侧(末位出屏)。
            lv_obj_set_pos(label, 12 + i * 76 + 8, 160);
            app->cast.slot_labels[i] = label;
        }
    } else {
        // 单张:一张大牌位 120x180 居中(与 full 套图像素 1:1)。
        lv_obj_t *slot = lv_obj_create(app->screen);
        lv_obj_remove_style_all(slot);
        lv_obj_set_pos(slot, 60, 36);
        lv_obj_set_size(slot, TAROT_ART_W, TAROT_ART_H);
        app->cast.slots[0] = slot;
        app->cast.slots[1] = NULL;
        app->cast.slots[2] = NULL;
        cast_make_back(app, 0);

        lv_obj_t *label = lv_label_create(app->screen);
        lv_obj_set_style_text_font(label, &tarot_font_16, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(TT_COLOR_PAPER_DIM), 0);
        lv_label_set_text(label, "此刻");
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 220);
        app->cast.slot_labels[0] = label;
    }

    app->cast.result_label = lv_label_create(app->screen);
    lv_obj_set_style_text_font(app->cast.result_label, &tarot_font_24, 0);
    lv_obj_set_style_text_color(app->cast.result_label,
                                lv_color_hex(TT_COLOR_PAPER), 0);
    lv_obj_set_width(app->cast.result_label, 216);
    lv_obj_set_style_text_align(app->cast.result_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(app->cast.result_label, 12, 240);
    lv_label_set_text(app->cast.result_label, "");

    app->cast.hint_label = tarot_hint_create(app->screen, 292,
                                             "OK 抽牌 · 长按返回");
}

// 翻开第 revealed 张:撤掉牌背,在卡位里画真实牌面,并给出结果行。
static void cast_reveal(struct tarot_app_s *app) {
    int slot = app->revealed;
    const tarot_draw_t *draw = &app->spread.draws[slot];
    const tarot_card_t *card = tarot_card_get(draw->card);
    if (app->cast.backs[slot]) {
        lv_obj_delete(app->cast.backs[slot]);
        app->cast.backs[slot] = NULL;
    }
    lv_obj_t *slot_obj = app->cast.slots[slot];
    int w = lv_obj_get_width(slot_obj);
    int h = lv_obj_get_height(slot_obj);
    // 翻开真实牌面:三牌阵小卡位用 mini 套图,单张大卡位用 full 套图;
    // LVGL 无法在无 PSRAM 板上缩放图片(transform 需离屏层),
    // 因此两档尺寸各生成一套 1:1 图。几何牌面仅作图缺失时的兜底。
    const lv_image_dsc_t *art =
        app->draw_count == TAROT_SPREAD_THREE
            ? tarot_art_mini_get(draw->card, draw->reversed)
            : tarot_art_get(draw->card, draw->reversed);
    if (art) {
        lv_obj_t *img = lv_image_create(slot_obj);
        lv_image_set_src(img, art);
        lv_obj_set_pos(img, 0, 0);
        lv_obj_set_style_border_width(img, 2, 0);
        lv_obj_set_style_border_color(
            img, lv_color_hex(draw->reversed ? TT_COLOR_CINNABAR
                                             : TT_COLOR_GOLD_DIM),
            0);
        lv_obj_set_style_radius(img, 4, 0);
        lv_obj_set_style_clip_corner(img, true, 0);
    } else {
        tarot_emblem_create(slot_obj, draw->card, draw->reversed, 0, 0, w, h);
    }
    if (draw->reversed) {
        lv_obj_set_style_border_color(slot_obj, lv_color_hex(TT_COLOR_CINNABAR), 0);
    }

    const char *orient = draw->reversed ? "逆位" : "正位";
    lv_label_set_text_fmt(app->cast.result_label, "%s · %s",
                          card ? card->name_zh : "?", orient);
    if (slot_label_ok(app)) {
        lv_obj_set_style_text_color(app->cast.slot_labels[slot],
                                    lv_color_hex(TT_COLOR_GOLD), 0);
    }

    app->revealed++;
    if (app->revealed >= app->draw_count) {
        if (app->draw_count == TAROT_SPREAD_SINGLE) {
            // 单张:稍作停留后自动进入牌面解读。
            lv_timer_t *t = lv_timer_create(cast_auto_next, CAST_AUTO_NEXT_MS, app);
            if (t == NULL) {
                ESP_LOGE(TAG, "自动切页定时器创建失败,直接进入牌面页");
                tarot_app_goto(app, TAROT_PAGE_CARD);
                tarot_app_notify(app);
            } else {
                app->auto_timer = t;
            }
        } else {
            lv_label_set_text(app->cast.hint_label, "OK 翻看牌面 · 长按返回");
        }
    }
}

// 单张翻完自动进入牌面页。定时器回调运行于 LVGL 任务,可直接请求切页。
static void cast_auto_next(lv_timer_t *timer) {
    struct tarot_app_s *app = s_cast_app;
    lv_timer_del(timer);
    if (app) app->auto_timer = NULL;
    tarot_app_goto(app, TAROT_PAGE_CARD);
    tarot_app_notify(app);  // 立即唤醒应用任务完成切页
}

static void cast_anim_stop(struct tarot_app_s *app, bool settle) {
    if (app->anim_timer) {
        lv_timer_del(app->anim_timer);
        app->anim_timer = NULL;
    }
    app->cast_busy = false;
    if (!settle) return;
    // 归位抖动的牌背。
    int slot = app->revealed;
    if (app->cast.backs[slot]) {
        lv_obj_set_pos(app->cast.backs[slot], 4, 4);
    }
    cast_reveal(app);
}

static void cast_anim_tick(lv_timer_t *timer) {
    struct tarot_app_s *app = s_cast_app;
    (void)timer;
    s_cast_ticks++;
    uint32_t rand = esp_random();
    int slot = app->revealed;
    if (app->cast.backs[slot]) {
        // 牌背在卡位内随机偏移 ±3px,模拟洗牌。
        int dx = (int)(rand % 7) - 3;
        int dy = (int)((rand >> 8) % 7) - 3;
        lv_obj_set_pos(app->cast.backs[slot], 4 + dx, 4 + dy);
    }
    if (s_cast_ticks >= CAST_ANIM_TICKS) {
        cast_anim_stop(app, true);
    }
}

static bool slot_label_ok(struct tarot_app_s *app) {
    int slot = app->revealed;
    return slot >= 0 && slot < TAROT_DRAW_MAX && app->cast.slot_labels[slot];
}

static void cast_key(struct tarot_app_s *app, bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (btn == BSP_BTN_OK && ev == BSP_BTN_CLICK) {
        if (app->cast_busy) return;
        if (app->revealed >= app->draw_count) {
            // 全部翻开:进入牌面页。
            tarot_app_goto(app, TAROT_PAGE_CARD);
            return;
        }
        // 第一张按下时完成一次完整洗牌(整个牌阵一次抽出)。
        if (!app->spread_valid) {
            uint32_t seed = esp_random();
            if (!tarot_draw_spread((uint8_t)app->draw_count, seed,
                                   &app->spread)) {
                ESP_LOGE(TAG, "抽牌失败");
                return;
            }
            app->spread_valid = true;
        }
        app->cast_busy = true;
        lv_label_set_text(app->cast.result_label, "");
        lv_timer_t *t = lv_timer_create(cast_anim_tick, CAST_ANIM_PERIOD_MS, app);
        if (t == NULL) {
            // 建不出定时器就不能播动画:直接翻开,让用户可以继续。
            ESP_LOGE(TAG, "洗牌动画定时器创建失败,直接翻牌");
            app->cast_busy = false;
            cast_reveal(app);
            return;
        }
        app->anim_timer = t;
        s_cast_ticks = 0;
    } else if (btn == BSP_BTN_OK && ev == BSP_BTN_LONG) {
        tarot_app_goto(app, TAROT_PAGE_HOME);
    }
}

static void cast_exit(struct tarot_app_s *app) {
    if (app->anim_timer) {
        lv_timer_del(app->anim_timer);
        app->anim_timer = NULL;
    }
    if (app->auto_timer) {
        lv_timer_del(app->auto_timer);
        app->auto_timer = NULL;
    }
    app->cast_busy = false;
    s_cast_app = NULL;
}

static const tarot_page_ops_t k_cast_ops = {cast_build, cast_key, cast_exit};

const tarot_page_ops_t *tarot_page_ops_cast(tarot_page_t state) {
    if (state == TAROT_PAGE_CAST) return &k_cast_ops;
    return NULL;
}
