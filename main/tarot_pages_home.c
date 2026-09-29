// main/tarot_pages_home.c —— 封面页:选择牌阵(单张 / 三牌阵)。
#include <stdio.h>

#include "bsp_button.h"
#include "tarot_app_internal.h"
#include "tarot_nav.h"
#include "tarot_theme.h"

static const char *const k_mode_titles[TAROT_MODE_COUNT] = {
    "单张抽牌", "三牌阵",
};
static const char *const k_mode_subs[TAROT_MODE_COUNT] = {
    "每日一牌,直指当下", "过去 · 现在 · 未来",
};

static void home_refresh(struct tarot_app_s *app) {
    for (int i = 0; i < TAROT_MODE_COUNT; i++) {
        lv_obj_t *panel = app->home.options[i];
        if (!panel) continue;
        bool selected = i == app->sel;
        tarot_style_option(panel, selected);
        // 标题继承面板字色(选中朱砂/未选宣纸);副标题固定次要色,
        // 选中时压成墨色以保证宣纸底上的对比度。
        lv_obj_t *title = lv_obj_get_child(panel, 0);
        lv_obj_t *sub = lv_obj_get_child(panel, 1);
        lv_obj_set_style_text_color(
            sub, lv_color_hex(selected ? TT_COLOR_INK : TT_COLOR_PAPER_DIM), 0);
        (void)title;
    }
}

static void home_build(struct tarot_app_s *app) {
    app->screen = tarot_page_create("塔罗占卜");
    app->sel = (int)app->mode;

    // 封面大字:48px 独立字库只含"塔罗"两字,flash 代价极小。
    lv_obj_t *hero = lv_label_create(app->screen);
    lv_obj_set_style_text_font(hero, &tarot_font_48, 0);
    lv_obj_set_style_text_color(hero, lv_color_hex(TT_COLOR_GOLD), 0);
    lv_label_set_text(hero, "塔罗");
    lv_obj_align(hero, LV_ALIGN_TOP_MID, 0, 56);

    lv_obj_t *sub = lv_label_create(app->screen);
    lv_obj_set_style_text_font(sub, &tarot_font_16, 0);
    lv_obj_set_style_text_color(sub, lv_color_hex(TT_COLOR_PAPER_DIM), 0);
    lv_label_set_text(sub, "七十八张 · 正逆位");
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 116);

    for (int i = 0; i < TAROT_MODE_COUNT; i++) {
        lv_obj_t *panel = lv_obj_create(app->screen);
        lv_obj_remove_style_all(panel);
        lv_obj_set_pos(panel, 24, 150 + i * 66);
        lv_obj_set_size(panel, 192, 58);
        lv_obj_t *title = lv_label_create(panel);
        lv_obj_set_style_text_font(title, &tarot_font_24, 0);
        lv_obj_set_pos(title, 14, 4);
        lv_label_set_text(title, k_mode_titles[i]);
        lv_obj_t *sub_label = lv_label_create(panel);
        lv_obj_set_style_text_font(sub_label, &tarot_font_16, 0);
        lv_obj_set_pos(sub_label, 14, 32);
        lv_label_set_text(sub_label, k_mode_subs[i]);
        app->home.options[i] = panel;
    }

    tarot_hint_create(app->screen, 292, "上下选择 · OK 开始");
    home_refresh(app);
}

static void home_key(struct tarot_app_s *app, bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (ev != BSP_BTN_CLICK && ev != BSP_BTN_LONG) return;
    if (btn == BSP_BTN_UP && ev == BSP_BTN_CLICK) {
        app->sel = tarot_mode_step(app->sel, 1);
        home_refresh(app);
    } else if (btn == BSP_BTN_DOWN && ev == BSP_BTN_CLICK) {
        app->sel = tarot_mode_step(app->sel, -1);
        home_refresh(app);
    } else if (btn == BSP_BTN_OK && ev == BSP_BTN_CLICK) {
        app->mode = (tarot_mode_t)app->sel;
        int count = tarot_mode_draw_count(app->sel);
        if (count <= 0) return;  // 防御:非法选择不进入抽牌页
        app->draw_count = count;
        app->revealed = 0;
        app->spread_valid = false;
        tarot_app_goto(app, TAROT_PAGE_CAST);
    }
    // OK 长按在封面无事可做:保持当前页,不重建。
}

static const tarot_page_ops_t k_home_ops = {home_build, home_key, NULL};

const tarot_page_ops_t *tarot_page_ops_home(tarot_page_t state) {
    if (state == TAROT_PAGE_HOME) return &k_home_ops;
    return NULL;
}
