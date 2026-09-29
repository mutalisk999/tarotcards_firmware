// main/tarot_emblem.c —— 塔罗牌面符号渲染实现。
// 原语规格来自 tarot_emblem_math(归一化、含逆位镜像),这里:
//   BAR  → 矩形           DOT → 实心圆        RING → 描边圆
//   ARC  → lv_arc 弧段(角度做 y 镜像换算)
//   POLY → lv_line 闭合轮廓(点在盒内做 y 镜像换算)
#include <stdbool.h>
#include <string.h>

#include "tarot_deck.h"
#include "tarot_emblem.h"
#include "tarot_emblem_math.h"
#include "tarot_theme.h"

static uint32_t role_color(tarot_prim_role_t role, int suit) {
    switch (role) {
        case TAROT_ROLE_GOLD: return TT_COLOR_GOLD;
        case TAROT_ROLE_PAPER: return TT_COLOR_PAPER;
        case TAROT_ROLE_DIM: return TT_COLOR_GOLD_DIM;
        case TAROT_ROLE_ACCENT: return TT_COLOR_CINNABAR;
        case TAROT_ROLE_BG: return TT_COLOR_BG;
        case TAROT_ROLE_SUIT: {
            static const uint32_t k_suit_colors[] = {
                TT_COLOR_SUIT_WANDS, TT_COLOR_SUIT_CUPS,
                TT_COLOR_SUIT_SWORDS, TT_COLOR_SUIT_PENTACLES,
            };
            return (suit >= 0 && suit < 4) ? k_suit_colors[suit] : TT_COLOR_GOLD;
        }
        default: return TT_COLOR_PAPER;
    }
}

static lv_obj_t *prim_bar(lv_obj_t *parent, const tarot_prim_spec_t *spec,
                          int x, int y, int w, int h, uint32_t color) {
    (void)spec;
    return tarot_rect_create(parent, x, y, w, h, color);
}

static lv_obj_t *prim_dot(lv_obj_t *parent, int x, int y, int w, int h,
                          uint32_t color) {
    // 盒内最大内切圆:直径取宽高较小者,居中。
    int d = w < h ? w : h;
    int r = d / 2;
    if (r < 1) r = 1;
    return tarot_circle_create(parent, x + w / 2, y + h / 2, r, color);
}

static lv_obj_t *prim_ring(lv_obj_t *parent, const tarot_prim_spec_t *spec,
                           int x, int y, int w, int h, uint32_t color) {
    int d = w < h ? w : h;
    int r = d / 2;
    if (r < 2) r = 2;
    int width = d / 12 > 1 ? d / 12 : 1;
    return tarot_ring_create(parent, x + w / 2, y + h / 2, r, width, color,
                             LV_OPA_COVER);
}

static lv_obj_t *prim_arc(lv_obj_t *parent, const tarot_prim_spec_t *spec,
                          int x, int y, int w, int h, bool reversed,
                          uint32_t color) {
    // LVGL 角度:0 度 = 3 点钟,顺时针为正。y 镜像等价于角度取反:
    // [start,end] → [-end,-start](LVGL 接受负角,内部归一化)。
    int start = (int)spec->aux;
    int end = (int)spec->aux2;
    if (reversed) {
        int mirrored_start = -end;
        end = -start;
        start = mirrored_start;
    }
    int d = w < h ? w : h;
    if (d < 4) d = 4;
    int width = d / 8 > 1 ? d / 8 : 1;
    return tarot_arc_create(parent, x + w / 2, y + h / 2, d / 2, width, color,
                            start, end);
}

static lv_obj_t *prim_poly(lv_obj_t *parent, const tarot_prim_spec_t *spec,
                           int x, int y, int w, int h, bool reversed,
                           uint32_t color) {
    const uint16_t *nx = NULL;
    const uint16_t *ny = NULL;
    int count = tarot_poly_points((int)spec->aux, &nx, &ny);
    if (count < 2) return NULL;

    // 盒内镜像:点的归一化 y 取反,再线性映射到镜像后的盒。
    lv_point_precise_t pts[13];
    if (count > 13) count = 13;
    for (int i = 0; i < count; i++) {
        pts[i].x = (lv_coord_t)(x + (int)nx[i] * w / TAROT_EMBLEM_NORM);
        int rel = reversed ? (TAROT_EMBLEM_NORM - (int)ny[i]) : (int)ny[i];
        pts[i].y = (lv_coord_t)(y + rel * h / TAROT_EMBLEM_NORM);
    }
    // 闭合轮廓:回到起点。
    if (count < 13) {
        pts[count] = pts[0];
        count++;
    }
    return tarot_poly_create(parent, pts, count, color, 2);
}

// 绘制一条符号原语。返回创建的部件(可能为 NULL:池耗尽等)。
static lv_obj_t *render_prim(lv_obj_t *parent, const tarot_prim_spec_t *spec,
                             int w, int h, bool reversed, int suit) {
    int x, y, pw, ph;
    tarot_prim_place(spec, w, h, reversed, &x, &y, &pw, &ph);
    uint32_t color = role_color((tarot_prim_role_t)spec->role, suit);
    switch ((tarot_prim_kind_t)spec->kind) {
        case TAROT_PRIM_BAR:
            return prim_bar(parent, spec, x, y, pw, ph, color);
        case TAROT_PRIM_DOT:
            return prim_dot(parent, x, y, pw, ph, color);
        case TAROT_PRIM_RING:
            return prim_ring(parent, spec, x, y, pw, ph, color);
        case TAROT_PRIM_ARC:
            return prim_arc(parent, spec, x, y, pw, ph, reversed, color);
        case TAROT_PRIM_POLY:
            return prim_poly(parent, spec, x, y, pw, ph, reversed, color);
        default:
            return NULL;
    }
}

// 归一化坐标辅助(供宫廷牌;与 tarot_prim_place 同一套镜像规则)。
static int x_of(uint16_t nx, int w) {
    return (int)nx * w / TAROT_EMBLEM_NORM;
}

static int y_of(uint16_t ny, uint16_t nh, int h, bool reversed) {
    if (reversed) {
        int bottom = ((int)ny + (int)nh) * h / TAROT_EMBLEM_NORM;
        return h - bottom;
    }
    return (int)ny * h / TAROT_EMBLEM_NORM;
}

// 小阿卡纳:把花色符号画进指定盒(数字牌点阵与宫廷牌共用)。
static void draw_suit_emblem(lv_obj_t *parent, int suit, int x, int y,
                             int w, int h, bool reversed) {
    const tarot_prim_spec_t *specs = NULL;
    int count = tarot_suit_spec(suit, &specs);
    for (int i = 0; i < count; i++) {
        render_prim(parent, &specs[i], w, h, reversed, suit);
        (void)specs;
    }
}

static void draw_major(lv_obj_t *parent, int id, int w, int h, bool reversed) {
    const tarot_prim_spec_t *specs = NULL;
    int count = tarot_major_spec(id, &specs);
    for (int i = 0; i < count; i++) {
        render_prim(parent, &specs[i], w, h, reversed, -1);
    }
}

static void draw_minor(lv_obj_t *parent, const tarot_card_t *card,
                       int w, int h, bool reversed) {
    if (card->rank >= 11) {
        // 宫廷牌:大花色符号 + 顶部徽记(侍从菱形/骑士三角/王后冠/国王十字)。
        uint16_t nx, ny, nw, nh;
        int insignia = -1;
        tarot_court_layout(card->rank, &nx, &ny, &nw, &nh, &insignia);
        int ex = x_of(nx, w);
        int ey = y_of(ny, nh, h, reversed);
        int ew = (int)nw * w / TAROT_EMBLEM_NORM;
        int eh = (int)nh * h / TAROT_EMBLEM_NORM;
        draw_suit_emblem(parent, card->suit, ex, ey, ew, eh, reversed);
        if (insignia >= 0) {
            // 徽记小盒置于符号盒上方(逆位时在下方,由 y_of 镜像决定)。
            int iw = ew * 44 / 100;
            int ih = eh * 22 / 100;
            int ix = ex + (ew - iw) / 2;
            int iy;
            if (reversed) {
                iy = ey + eh + eh * 6 / 100;
                if (iy + ih > h) iy = ey - ih - eh * 6 / 100;
            } else {
                iy = ey - ih - eh * 6 / 100;
                if (iy < 0) iy = ey + eh + eh * 6 / 100;
            }
            const uint16_t *px = NULL;
            const uint16_t *py = NULL;
            int pc = tarot_poly_points(insignia, &px, &py);
            if (pc >= 2) {
                lv_point_precise_t pts[13];
                if (pc > 12) pc = 12;
                for (int i = 0; i < pc; i++) {
                    pts[i].x = (lv_coord_t)(ix + (int)px[i] * iw / TAROT_EMBLEM_NORM);
                    int rel = reversed ? (TAROT_EMBLEM_NORM - (int)py[i])
                                       : (int)py[i];
                    pts[i].y = (lv_coord_t)(iy + rel * ih / TAROT_EMBLEM_NORM);
                }
                pts[pc] = pts[0];
                tarot_poly_create(parent, pts, pc + 1, TT_COLOR_GOLD, 2);
            }
        }
        return;
    }

    // 数字牌:似扑克点阵,每个点位画一个缩小的花色符号。
    const uint16_t *xs = NULL;
    const uint16_t *ys = NULL;
    int pips = tarot_pip_layout(card->rank, &xs, &ys);
    // 点位跨 220..780(56% 宽),符号盒取该跨度的 42%,避免相邻粘连。
    int sw = w * 56 / 100 * 42 / 100;
    int sh = h * 56 / 100 * 42 / 100;
    if (sw < 8) sw = 8;
    if (sh < 8) sh = 8;
    for (int i = 0; i < pips; i++) {
        // 符号舞台以自身 (0,0) 为原点:点位即舞台内坐标。
        int cx = (int)xs[i] * w / TAROT_EMBLEM_NORM;
        int cy_norm = reversed ? (TAROT_EMBLEM_NORM - (int)ys[i]) : (int)ys[i];
        int cy = cy_norm * h / TAROT_EMBLEM_NORM;
        draw_suit_emblem(parent, card->suit, cx - sw / 2, cy - sh / 2, sw, sh,
                         reversed);
    }
}

lv_obj_t *tarot_emblem_create(lv_obj_t *parent, int card_id, bool reversed,
                              int x, int y, int w, int h) {
    if (w < 24 || h < 24) return NULL;
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_remove_style_all(box);
    lv_obj_set_pos(box, x, y);
    lv_obj_set_size(box, w, h);

    // 牌框:暗金细边圆角 + 略深底,像一张背面朝上的牌托。
    lv_obj_set_style_bg_color(box, lv_color_hex(TT_COLOR_PANEL), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(box, 8, 0);
    lv_obj_set_style_border_width(box, 2, 0);
    lv_obj_set_style_border_color(box, lv_color_hex(TT_COLOR_GOLD_DIM), 0);

    // 内符号区:留出边框与内边距。
    const int pad = 5;
    int iw = w - pad * 2;
    int ih = h - pad * 2;

    const tarot_card_t *card = tarot_card_get(card_id);
    if (!card) return box;
    if (iw < 30 || ih < 30) {
        // 过小:只画一个花色色点示意,避免符号挤成一团。
        tarot_circle_create(box, w / 2, h / 2, (iw < ih ? iw : ih) / 3,
                            role_color(TAROT_ROLE_SUIT,
                                       tarot_card_is_major(card_id)
                                           ? -1
                                           : (int)card->suit));
        return box;
    }

    // 折线点池随旧屏删除而失效:构建新牌面前统一复位。
    tarot_theme_reset_poly_pool();

    // 符号区内再留 10% 呼吸边距,让构图不顶框。
    int sx = pad + iw / 10;
    int sy = pad + ih / 10;
    int sw = iw - iw / 5;
    int sh = ih - ih / 5;

    lv_obj_t *stage = lv_obj_create(box);
    lv_obj_remove_style_all(stage);
    lv_obj_set_pos(stage, sx, sy);
    lv_obj_set_size(stage, sw, sh);

    if (tarot_card_is_major(card_id)) {
        draw_major(stage, card_id, sw, sh, reversed);
    } else {
        draw_minor(stage, card, sw, sh, reversed);
    }
    return box;
}

    // 数字牌:似扑克点阵,每个点位画一个缩小的花色符号。