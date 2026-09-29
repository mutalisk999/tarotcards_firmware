// main/tarot_theme.c —— 塔罗应用视觉主题实现。
#include "tarot_theme.h"

#include <string.h>

// 折线点静态池:单张牌面至多 2 条折线轮廓,每条至多 12 点 + 1 闭合点。
// lv_line 的点数组必须在对象存续期内有效,栈数组不行;池按对象数分配,
// 池耗尽时放弃绘制(与 LVGL 内存不足同级的最坏表现是缺一个装饰)。
#define TT_POLY_SLOTS 4
#define TT_POLY_POINTS_MAX 13

static lv_point_precise_t s_poly_pool[TT_POLY_SLOTS][TT_POLY_POINTS_MAX];
static bool s_poly_used[TT_POLY_SLOTS];

static lv_obj_t *rect(lv_obj_t *parent, int x, int y, int w, int h,
                      uint32_t color) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj, 2, 0);
    return obj;
}

lv_obj_t *tarot_rect_create(lv_obj_t *parent, int x, int y, int w, int h,
                            uint32_t color) {
    return rect(parent, x, y, w, h, color);
}

lv_obj_t *tarot_hairline(lv_obj_t *parent, int x, int y, int w) {
    return rect(parent, x, y, w, 1, TT_COLOR_PANEL_2);
}

// 页面四角暗金角饰:横竖两条,低透明度,不与内容争抢注意力。
void tarot_corner_ornaments(lv_obj_t *parent) {
    const int len = 12, t = 2, m = 6;
    const int co[4][8] = {
        {m, m, len, t, m, m, t, len},                            // 左上
        {240 - m - len, m, len, t, 240 - m - t, m, t, len},      // 右上
        {m, 320 - m - t, len, t, m, 320 - m - len, t, len},      // 左下
        {240 - m - len, 320 - m - t, len, t, 240 - m - t,
         320 - m - len, t, len},                                 // 右下
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *a = rect(parent, co[i][0], co[i][1], co[i][2], co[i][3],
                           TT_COLOR_GOLD_DIM);
        lv_obj_set_style_bg_opa(a, LV_OPA_60, 0);
        lv_obj_t *b = rect(parent, co[i][4], co[i][5], co[i][6], co[i][7],
                           TT_COLOR_GOLD_DIM);
        lv_obj_set_style_bg_opa(b, LV_OPA_60, 0);
    }
}

lv_obj_t *tarot_hint_create(lv_obj_t *parent, int y, const char *text) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, &tarot_font_16, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(TT_COLOR_PAPER_DIM), 0);
    lv_label_set_text(label, text);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, y);
    return label;
}

lv_obj_t *tarot_page_create(const char *title) {
    lv_obj_t *root = lv_obj_create(NULL);
    // 纵向渐变星夜底:上亮下暗,营造纵深。
    lv_obj_set_style_bg_color(root, lv_color_hex(TT_COLOR_BG), 0);
    lv_obj_set_style_bg_grad_color(root, lv_color_hex(TT_COLOR_BG_2), 0);
    lv_obj_set_style_bg_grad_dir(root, LV_GRAD_DIR_VER, 0);
    tarot_corner_ornaments(root);

    if (title) {
        // 标题鎏金竖条 + 细分隔线,确立页面骨架。
        rect(root, 12, 8, 3, 16, TT_COLOR_GOLD);
        lv_obj_t *label = lv_label_create(root);
        lv_obj_set_style_text_font(label, &tarot_font_24, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(TT_COLOR_PAPER), 0);
        lv_label_set_text(label, title);
        lv_obj_set_pos(label, 22, 4);
        tarot_hairline(root, 12, 34, 216);
    }
    return root;
}

void tarot_battery_update(lv_obj_t *battery_label, int soc) {
    if (!battery_label) return;
    if (soc < 0) {
        lv_label_set_text(battery_label, "--");
    } else {
        lv_label_set_text_fmt(battery_label, "%d%%", soc);
    }
    lv_obj_set_style_text_color(battery_label,
                                soc >= 0 && soc < 20
                                    ? lv_color_hex(TT_COLOR_CINNABAR)
                                    : lv_color_hex(TT_COLOR_PAPER_DIM),
                                0);
}

lv_obj_t *tarot_circle_create(lv_obj_t *parent, int cx, int cy, int radius,
                              uint32_t color) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, radius * 2, radius * 2);
    lv_obj_set_pos(obj, cx - radius, cy - radius);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    return obj;
}

lv_obj_t *tarot_ring_create(lv_obj_t *parent, int cx, int cy, int radius,
                            int width, uint32_t color, lv_opa_t opa) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, radius * 2, radius * 2);
    lv_obj_set_pos(obj, cx - radius, cy - radius);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, width, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_border_opa(obj, opa, 0);
    return obj;
}

lv_obj_t *tarot_arc_create(lv_obj_t *parent, int cx, int cy, int radius,
                           int width, uint32_t color, int start_angle,
                           int end_angle) {
    // lv_arc 的背景角决定弧段范围;指示器填满背景角,旋钮移除。
    int d = radius * 2;
    if (d < 2) d = 2;
    lv_obj_t *obj = lv_arc_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, d, d);
    lv_obj_set_pos(obj, cx - d / 2, cy - d / 2);
    lv_arc_set_rotation(obj, 0);
    lv_arc_set_bg_angles(obj, start_angle, end_angle);
    lv_arc_set_value(obj, 100);
    lv_obj_set_style_arc_width(obj, width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(obj, lv_color_hex(color), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(obj, true, LV_PART_INDICATOR);
    lv_obj_remove_style(obj, NULL, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    return obj;
}

lv_obj_t *tarot_poly_create(lv_obj_t *parent, const lv_point_precise_t *points,
                            int count, uint32_t color, int line_width) {
    if (!points || count < 2 || count > TT_POLY_POINTS_MAX) return NULL;
    int slot = -1;
    for (int i = 0; i < TT_POLY_SLOTS; i++) {
        if (!s_poly_used[i]) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return NULL;  // 池耗尽:放弃这条装饰轮廓

    memcpy(s_poly_pool[slot], points, (size_t)count * sizeof(lv_point_t));
    s_poly_used[slot] = true;

    lv_obj_t *obj = lv_line_create(parent);
    // 池点数组永不回收(装饰总量有限):牌面页随屏幕删除,
    // 池槽位在每次构建牌面前统一复位(见 tarot_emblem_reset())。
    lv_line_set_points(obj, s_poly_pool[slot], count);
    lv_obj_set_style_line_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_line_width(obj, line_width, 0);
    lv_obj_set_style_line_rounded(obj, true, 0);
    return obj;
}

void tarot_theme_reset_poly_pool(void) {
    memset(s_poly_used, 0, sizeof(s_poly_used));
}
