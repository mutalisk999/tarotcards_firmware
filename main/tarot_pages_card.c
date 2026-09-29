// main/tarot_pages_card.c —— 牌面解读页:RWS 艺术牌面 + 正逆位关键词与短解读。
// 三牌阵:OK 依次切换 过去/现在/未来;上下键滚动解读文本;长按回封面。
// 逆位显示上下颠倒的真图(行翻转副本),配朱砂描边与"逆位"徽记。
#include <stdio.h>
#include <string.h>

#include "bsp_button.h"
#include "esp_log.h"
#include "tarot_app_internal.h"
#include "tarot_art.h"
#include "tarot_deck.h"
#include "tarot_nav.h"
#include "tarot_theme.h"

static const char *TAG = "tarot-card";

#define CARD_ART_W TAROT_ART_W   // 120
#define CARD_ART_H TAROT_ART_H   // 180
#define CARD_ART_X 8
#define CARD_ART_Y 38

// 无有效牌阵时(异常路径进入)不渲染空页,直接回封面。
static bool ensure_valid(struct tarot_app_s *app) {
    if (app->spread_valid) return true;
    ESP_LOGE(TAG, "牌面页收到无效牌阵,返回封面");
    tarot_app_goto(app, TAROT_PAGE_HOME);
    return false;
}

static void card_refresh_tabs(struct tarot_app_s *app) {
    if (app->draw_count != TAROT_SPREAD_THREE) return;
    for (int i = 0; i < app->draw_count; i++) {
        lv_obj_t *tab = app->card.tabs[i];
        if (!tab) continue;
        bool current = i == app->card_index;
        lv_obj_set_style_text_color(
            tab, lv_color_hex(current ? TT_COLOR_GOLD : TT_COLOR_PAPER_DIM), 0);
    }
}

static void card_refresh(struct tarot_app_s *app) {
    const tarot_draw_t *draw = &app->spread.draws[app->card_index];
    const tarot_card_t *card = tarot_card_get(draw->card);

    // 牌面艺术图:每次翻牌重建(旧图对象随删除释放)。
    // 逆位图渲染进静态 RAM 缓冲,一次只保一张,与单图显示方式一致。
    if (app->card.emblem) {
        lv_obj_delete(app->card.emblem);
        app->card.emblem = NULL;
    }
    const lv_image_dsc_t *art = tarot_art_get(draw->card, draw->reversed);
    if (art) {
        lv_obj_t *img = lv_image_create(app->screen);
        lv_image_set_src(img, art);
        lv_obj_set_pos(img, CARD_ART_X, CARD_ART_Y);
        // 朱砂细框:正位低调分隔,逆位强化"这张牌倒了"的辨识。
        lv_obj_set_style_border_width(img, 2, 0);
        lv_obj_set_style_border_color(
            img, lv_color_hex(draw->reversed ? TT_COLOR_CINNABAR
                                             : TT_COLOR_GOLD_DIM),
            0);
        lv_obj_set_style_radius(img, 4, 0);
        lv_obj_set_style_clip_corner(img, true, 0);
        app->card.emblem = img;
    }

    if (card) {
        lv_label_set_text(app->card.name_label, card->name_zh);
        lv_label_set_text(app->card.kw_label,
                          draw->reversed ? card->keywords_rev
                                         : card->keywords_up);
        lv_label_set_text(app->card.body,
                          draw->reversed ? card->meaning_rev
                                         : card->meaning_up);
    }
    lv_label_set_text(app->card.orient_label, draw->reversed ? "逆位" : "正位");
    lv_obj_set_style_text_color(
        app->card.orient_label,
        lv_color_hex(draw->reversed ? TT_COLOR_CINNABAR : TT_COLOR_GOLD), 0);

    lv_obj_scroll_to(app->card.scroll, 0, 0, LV_ANIM_OFF);
    card_refresh_tabs(app);
}

static void card_build(struct tarot_app_s *app) {
    if (!ensure_valid(app)) return;
    if (app->card_index < 0 || app->card_index >= app->draw_count) {
        app->card_index = 0;
    }
    app->screen = tarot_page_create("牌面解读");

    // 三牌阵:位置页签行(单张隐藏),置于右列顶端,避让左侧 120px 宽的牌面。
    if (app->draw_count == TAROT_SPREAD_THREE) {
        static const char *const k_positions[3] = {"过去", "现在", "未来"};
        for (int i = 0; i < TAROT_SPREAD_THREE; i++) {
            lv_obj_t *tab = lv_label_create(app->screen);
            lv_obj_set_style_text_font(tab, &tarot_font_16, 0);
            lv_label_set_text(tab, k_positions[i]);
            lv_obj_set_pos(tab, 134 + i * 36, 38);
            app->card.tabs[i] = tab;
        }
    } else {
        app->card.tabs[0] = app->card.tabs[1] = app->card.tabs[2] = NULL;
    }

    // 右列(x=134,宽 102):牌名 / 正逆位 / 关键词。
    app->card.name_label = lv_label_create(app->screen);
    lv_obj_set_style_text_font(app->card.name_label, &tarot_font_24, 0);
    lv_obj_set_style_text_color(app->card.name_label,
                                lv_color_hex(TT_COLOR_PAPER), 0);
    lv_label_set_long_mode(app->card.name_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(app->card.name_label, 102);
    lv_obj_set_pos(app->card.name_label, 134, 62);

    app->card.orient_label = lv_label_create(app->screen);
    lv_obj_set_style_text_font(app->card.orient_label, &tarot_font_16, 0);
    lv_obj_set_pos(app->card.orient_label, 134, 96);

    app->card.kw_label = lv_label_create(app->screen);
    lv_obj_set_style_text_font(app->card.kw_label, &tarot_font_16, 0);
    lv_obj_set_style_text_color(app->card.kw_label,
                                lv_color_hex(TT_COLOR_PAPER_DIM), 0);
    lv_label_set_long_mode(app->card.kw_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(app->card.kw_label, 102);
    lv_obj_set_pos(app->card.kw_label, 134, 118);

    // 解读区:墨面板 + 细边圆角,上下键滚动(不透明,避免掩码图层)。
    lv_obj_t *scroll = lv_obj_create(app->screen);
    lv_obj_remove_style_all(scroll);
    lv_obj_set_pos(scroll, 12, 224);
    lv_obj_set_size(scroll, 216, 68);
    lv_obj_set_scroll_dir(scroll, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(scroll, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(scroll, lv_color_hex(TT_COLOR_PANEL), 0);
    lv_obj_set_style_bg_opa(scroll, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(scroll, 8, 0);
    lv_obj_set_style_border_width(scroll, 1, 0);
    lv_obj_set_style_border_color(scroll, lv_color_hex(TT_COLOR_PANEL_2), 0);
    lv_obj_set_style_pad_all(scroll, 6, 0);
    lv_obj_t *body = lv_label_create(scroll);
    lv_obj_set_style_text_font(body, &tarot_font_16, 0);
    lv_obj_set_style_text_color(body, lv_color_hex(TT_COLOR_PAPER), 0);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(body, 200);  // 固定宽度:避免布局反馈循环
    lv_label_set_text(body, "");
    app->card.scroll = scroll;
    app->card.body = body;

    tarot_hint_create(app->screen, 296,
                      app->draw_count == TAROT_SPREAD_THREE
                          ? "上下滚动 · OK 下一张 · 长按返回"
                          : "上下滚动 · 长按返回");
    card_refresh(app);
}

static void card_key(struct tarot_app_s *app, bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (btn == BSP_BTN_UP && ev == BSP_BTN_CLICK) {
        lv_obj_scroll_by(app->card.scroll, 0, 36, LV_ANIM_OFF);
    } else if (btn == BSP_BTN_DOWN && ev == BSP_BTN_CLICK) {
        lv_obj_scroll_by(app->card.scroll, 0, -36, LV_ANIM_OFF);
    } else if (btn == BSP_BTN_OK && ev == BSP_BTN_CLICK) {
        if (app->draw_count == TAROT_SPREAD_THREE) {
            int next = tarot_card_step(app->card_index, app->draw_count, 1);
            if (next >= 0) {
                app->card_index = next;
                card_refresh(app);
            }
        }
        // 单张模式:OK 短按无事可做(解读就在本页滚动)。
    } else if (btn == BSP_BTN_OK && ev == BSP_BTN_LONG) {
        tarot_app_goto(app, TAROT_PAGE_HOME);
    }
}

static const tarot_page_ops_t k_card_ops = {card_build, card_key, NULL};

const tarot_page_ops_t *tarot_page_ops_card(tarot_page_t state) {
    if (state == TAROT_PAGE_CARD) return &k_card_ops;
    return NULL;
}
