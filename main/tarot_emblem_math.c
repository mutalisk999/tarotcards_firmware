// main/tarot_emblem_math.c —— 塔罗牌面符号几何:原语规格、轮廓点表、点阵布局。
// 全部为纯数据与纯函数;渲染层(tarot_emblem.c)只做"规格 → LVGL 部件"。
#include "tarot_emblem_math.h"

// ---------------------------------------------------------------------------
// 轮廓点表(归一化 0..1000,相对 POLY 自身盒子;渲染时闭合)。
// 下标即 POLY 原语的 aux 值,顺序勿改:
// 0 三角  1 倒三角  2 菱形  3 五角星  4 王冠  5 闪电  6 十字
// ---------------------------------------------------------------------------
typedef struct {
    const uint16_t *xs;
    const uint16_t *ys;
    int count;
} poly_table_t;

static const uint16_t k_tri_xs[] = {500, 1000, 0};
static const uint16_t k_tri_ys[] = {0, 1000, 1000};
static const uint16_t k_tridown_xs[] = {0, 1000, 500};
static const uint16_t k_tridown_ys[] = {0, 0, 1000};
static const uint16_t k_diamond_xs[] = {500, 1000, 500, 0};
static const uint16_t k_diamond_ys[] = {0, 500, 1000, 500};
// 五角星:外接半径 500、内半径 190,顶点朝上。
static const uint16_t k_star5_xs[] = {500, 612, 976, 681, 794,
                                      500, 206, 319, 24,  388};
static const uint16_t k_star5_ys[] = {0,   346, 346, 559, 904,
                                      690, 904, 559, 346, 346};
// 王冠。
static const uint16_t k_crown_xs[] = {0, 0, 250, 500, 750, 1000, 1000};
static const uint16_t k_crown_ys[] = {1000, 250, 600, 150, 600, 250, 1000};
// 闪电(闭合六边形)。
static const uint16_t k_bolt_xs[] = {620, 150, 430, 380, 850, 560};
static const uint16_t k_bolt_ys[] = {0, 560, 560, 1000, 420, 420};
// 十字(轮廓,12 点)。
static const uint16_t k_cross_xs[] = {350, 650, 650, 1000, 1000, 650,
                                      650, 350, 350, 0,    0,    350};
static const uint16_t k_cross_ys[] = {0,    0,    280, 280,  580,  580,
                                      1000, 1000, 580, 580,  280,  280};

static const poly_table_t k_polys[] = {
    {k_tri_xs, k_tri_ys, 3},
    {k_tridown_xs, k_tridown_ys, 3},
    {k_diamond_xs, k_diamond_ys, 4},
    {k_star5_xs, k_star5_ys, 10},
    {k_crown_xs, k_crown_ys, 7},
    {k_bolt_xs, k_bolt_ys, 6},
    {k_cross_xs, k_cross_ys, 12},
};
#define POLY_COUNT (int)(sizeof(k_polys) / sizeof(k_polys[0]))

int tarot_poly_points(int index, const uint16_t **xs, const uint16_t **ys) {
    if (xs) *xs = NULL;
    if (ys) *ys = NULL;
    if (index < 0 || index >= POLY_COUNT) return 0;
    if (xs) *xs = k_polys[index].xs;
    if (ys) *ys = k_polys[index].ys;
    return k_polys[index].count;
}

// ---------------------------------------------------------------------------
// 归一化 → 像素映射。先线性映射 x,再镜像 y(逆位)。
// ---------------------------------------------------------------------------
void tarot_prim_place(const tarot_prim_spec_t *spec, int w, int h,
                      bool reversed, int *out_x, int *out_y,
                      int *out_w, int *out_h) {
    if (spec == NULL || w <= 0 || h <= 0) {
        if (out_x) *out_x = 0;
        if (out_y) *out_y = 0;
        if (out_w) *out_w = 0;
        if (out_h) *out_h = 0;
        return;
    }
    int px = (int)spec->nx * w / TAROT_EMBLEM_NORM;
    int pw = (int)spec->nw * w / TAROT_EMBLEM_NORM;
    int ph = (int)spec->nh * h / TAROT_EMBLEM_NORM;
    int py;
    if (reversed) {
        // 镜像后盒子顶边 = h - (ny+nh)*h/1000。
        int bottom = ((int)spec->ny + (int)spec->nh) * h / TAROT_EMBLEM_NORM;
        py = h - bottom;
    } else {
        py = (int)spec->ny * h / TAROT_EMBLEM_NORM;
    }
    if (pw < 1) pw = 1;
    if (ph < 1) ph = 1;
    if (out_x) *out_x = px;
    if (out_y) *out_y = py;
    if (out_w) *out_w = pw;
    if (out_h) *out_h = ph;
}

// ---------------------------------------------------------------------------
// 大阿卡纳专属构图(22 组,每组 ≤8 原语,坐标 0..1000)。
// 构图只求"几何可辨、牌牌不同":轴对齐条、圆/环/弧与折线轮廓。
// ---------------------------------------------------------------------------
static const tarot_prim_spec_t k_major_specs[][8] = {
    // 0 愚者:日轮高悬、崖边行人。
    {{TAROT_PRIM_RING, TAROT_ROLE_GOLD, 0, 290, 80, 420, 420, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 0, 770, 1000, 50, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_ACCENT, 0, 600, 640, 90, 90, 0}},
    // 1 魔术师:横八字(两环)与立杖。
    {{TAROT_PRIM_RING, TAROT_ROLE_GOLD, 0, 90, 180, 380, 380, 0},
     {TAROT_PRIM_RING, TAROT_ROLE_GOLD, 0, 470, 180, 380, 380, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 830, 120, 40, 700, 0}},
    // 2 女祭司:双柱之间一轮月亮。
    {{TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 110, 130, 90, 720, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 800, 130, 90, 720, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_GOLD, 0, 370, 620, 260, 260, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_BG, 0, 440, 570, 250, 250, 0}},
    // 3 女皇:王冠与丰饶之圆。
    {{TAROT_PRIM_POLY, TAROT_ROLE_GOLD, 4, 240, 120, 520, 340, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_GOLD, 0, 390, 540, 220, 220, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 150, 840, 700, 40, 0}},
    // 4 皇帝:纹章菱盾与十字。
    {{TAROT_PRIM_POLY, TAROT_ROLE_GOLD, 2, 210, 140, 580, 700, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 470, 240, 60, 500, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 300, 460, 400, 60, 0}},
    // 5 教皇:三层冠与顶十字。
    {{TAROT_PRIM_BAR, TAROT_ROLE_GOLD, 0, 290, 330, 420, 70, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_GOLD, 0, 290, 460, 420, 70, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_GOLD, 0, 290, 590, 420, 70, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 480, 60, 40, 240, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 400, 130, 200, 40, 0}},
    // 6 恋人:双环相扣,上方一点星光。
    {{TAROT_PRIM_RING, TAROT_ROLE_GOLD, 0, 120, 340, 420, 420, 0},
     {TAROT_PRIM_RING, TAROT_ROLE_GOLD, 0, 460, 340, 420, 420, 0},
     {TAROT_PRIM_POLY, TAROT_ROLE_ACCENT, 3, 430, 60, 140, 140, 0}},
    // 7 战车:双轮与顶篷,篷上一星。
    {{TAROT_PRIM_RING, TAROT_ROLE_GOLD, 0, 80, 560, 320, 320, 0},
     {TAROT_PRIM_RING, TAROT_ROLE_GOLD, 0, 600, 560, 320, 320, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 80, 300, 840, 70, 0},
     {TAROT_PRIM_POLY, TAROT_ROLE_DIM, 3, 420, 90, 160, 160, 0}},
    // 8 力量:横八字下的一弯温柔掌弧。
    {{TAROT_PRIM_RING, TAROT_ROLE_GOLD, 0, 230, 110, 300, 260, 0},
     {TAROT_PRIM_RING, TAROT_ROLE_GOLD, 0, 470, 110, 300, 260, 0},
     {TAROT_PRIM_ARC, TAROT_ROLE_GOLD, 0, 260, 520, 480, 420, 180}},
    // 9 隐士:提灯与杖。
    {{TAROT_PRIM_POLY, TAROT_ROLE_GOLD, 2, 300, 150, 400, 400, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_ACCENT, 0, 460, 310, 80, 80, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 760, 110, 36, 760, 0}},
    // 10 命运之轮:轮、十字辐条与轴心。
    {{TAROT_PRIM_RING, TAROT_ROLE_GOLD, 0, 150, 150, 700, 700, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 480, 150, 40, 700, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 150, 480, 700, 40, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_ACCENT, 0, 430, 430, 140, 140, 0}},
    // 11 正义:立柱横梁与两只天秤环。
    {{TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 480, 110, 40, 720, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 200, 230, 600, 40, 0},
     {TAROT_PRIM_RING, TAROT_ROLE_GOLD, 0, 110, 330, 180, 180, 0},
     {TAROT_PRIM_RING, TAROT_ROLE_GOLD, 0, 710, 330, 180, 180, 0}},
    // 12 倒吊人:顶梁垂足,头部在下,光环居中。
    {{TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 150, 110, 700, 50, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_GOLD, 0, 470, 160, 60, 240, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_GOLD, 0, 400, 640, 200, 200, 0},
     {TAROT_PRIM_RING, TAROT_ROLE_ACCENT, 0, 340, 420, 320, 150, 0}},
    // 13 死神:镰刀弧刃、立杆与一朵玫瑰。
    {{TAROT_PRIM_ARC, TAROT_ROLE_GOLD, 20, 150, 130, 640, 420, 160},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 770, 130, 36, 740, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_ACCENT, 0, 420, 640, 140, 140, 0}},
    // 14 节制:两只杯与中间落下的水滴。
    {{TAROT_PRIM_ARC, TAROT_ROLE_GOLD, 0, 120, 220, 320, 240, 180},
     {TAROT_PRIM_ARC, TAROT_ROLE_GOLD, 180, 560, 540, 320, 240, 360},
     {TAROT_PRIM_POLY, TAROT_ROLE_ACCENT, 2, 440, 400, 120, 180, 0}},
    // 15 恶魔:双角之间一颗五角星。
    {{TAROT_PRIM_POLY, TAROT_ROLE_ACCENT, 3, 250, 280, 500, 500, 0},
     {TAROT_PRIM_POLY, TAROT_ROLE_GOLD, 0, 170, 60, 190, 190, 0},
     {TAROT_PRIM_POLY, TAROT_ROLE_GOLD, 0, 640, 60, 190, 190, 0}},
    // 16 高塔:塔身、劈下的闪电与倾落的冠。
    {{TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 380, 300, 240, 620, 0},
     {TAROT_PRIM_POLY, TAROT_ROLE_ACCENT, 5, 150, 60, 700, 430, 0},
     {TAROT_PRIM_POLY, TAROT_ROLE_GOLD, 1, 360, 160, 280, 110, 0}},
    // 17 星星:一颗大星与七点小星。
    {{TAROT_PRIM_POLY, TAROT_ROLE_GOLD, 3, 300, 200, 400, 400, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_PAPER, 0, 110, 110, 60, 60, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_PAPER, 0, 760, 130, 60, 60, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_PAPER, 0, 830, 430, 60, 60, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_PAPER, 0, 130, 480, 60, 60, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_PAPER, 0, 590, 70, 50, 50, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_PAPER, 0, 80, 710, 50, 50, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_PAPER, 0, 840, 730, 50, 50, 0}},
    // 18 月亮:弦月、双塔与夜路。
    {{TAROT_PRIM_DOT, TAROT_ROLE_GOLD, 0, 300, 110, 360, 360, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_BG, 0, 380, 70, 340, 340, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 110, 540, 90, 320, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 800, 540, 90, 320, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_DIM, 0, 0, 900, 1000, 36, 0}},
    // 19 太阳:日轮与四向光芒。
    {{TAROT_PRIM_DOT, TAROT_ROLE_GOLD, 0, 290, 290, 420, 420, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 480, 50, 40, 180, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 480, 770, 40, 180, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 50, 480, 180, 40, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 770, 480, 180, 40, 0}},
    // 20 审判:号角、号管与棺盖上的回响。
    {{TAROT_PRIM_POLY, TAROT_ROLE_GOLD, 0, 140, 200, 430, 350, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 560, 260, 320, 50, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_PAPER, 0, 300, 660, 400, 60, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_ACCENT, 0, 700, 440, 110, 110, 0}},
    // 21 世界:椭圆桂冠、中心之星与四角点。
    {{TAROT_PRIM_RING, TAROT_ROLE_GOLD, 0, 190, 130, 620, 740, 0},
     {TAROT_PRIM_POLY, TAROT_ROLE_PAPER, 3, 390, 380, 220, 220, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_DIM, 0, 140, 100, 90, 90, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_DIM, 0, 770, 100, 90, 90, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_DIM, 0, 140, 810, 90, 90, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_DIM, 0, 770, 810, 90, 90, 0}},
};
#define MAJOR_SPEC_ROWS \
    (int)(sizeof(k_major_specs) / sizeof(k_major_specs[0]))
#define MAJOR_SPEC_CAP (int)(sizeof(k_major_specs[0]) / sizeof(k_major_specs[0][0]))

// 每组构图的实际原语数(其余槽位保持全零,不参与渲染)。
static const uint8_t k_major_counts[MAJOR_SPEC_ROWS] = {
    3, 3, 4, 3, 3, 5, 3, 4, 3, 3, 4, 4, 4, 3, 3, 3, 3, 8, 5, 5, 4, 6,
};

int tarot_major_spec(int id, const tarot_prim_spec_t **out) {
    if (out) *out = NULL;
    if (id < 0 || id >= MAJOR_SPEC_ROWS) return 0;
    if (out) *out = k_major_specs[id];
    return k_major_counts[id];
}

// ---------------------------------------------------------------------------
// 花色符号(4 组,同一构图可在任意尺寸盒子渲染;主体用 TAROT_ROLE_SUIT)。
// ---------------------------------------------------------------------------
static const tarot_prim_spec_t k_suit_specs[TAROT_EMBLEM_SUIT_COUNT][5] = {
    // 权杖:立杖与两片叶。
    {{TAROT_PRIM_BAR, TAROT_ROLE_SUIT, 0, 460, 60, 80, 880, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_SUIT, 0, 210, 240, 150, 150, 0},
     {TAROT_PRIM_DOT, TAROT_ROLE_SUIT, 0, 640, 430, 150, 150, 0}},
    // 圣杯:杯身、杯柄与底座。
    {{TAROT_PRIM_ARC, TAROT_ROLE_SUIT, 0, 150, 170, 700, 360, 180},
     {TAROT_PRIM_BAR, TAROT_ROLE_SUIT, 0, 470, 530, 60, 250, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_SUIT, 0, 290, 780, 420, 60, 0}},
    // 宝剑:剑尖、剑身、护手、柄与首。
    {{TAROT_PRIM_POLY, TAROT_ROLE_SUIT, 0, 410, 0, 180, 130, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_SUIT, 0, 470, 130, 60, 480, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_SUIT, 0, 290, 610, 420, 60, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_SUIT, 0, 460, 670, 80, 190, 0},
     {TAROT_PRIM_BAR, TAROT_ROLE_SUIT, 0, 380, 860, 240, 50, 0}},
    // 星币:外环内星。
    {{TAROT_PRIM_RING, TAROT_ROLE_SUIT, 0, 50, 50, 900, 900, 0},
     {TAROT_PRIM_POLY, TAROT_ROLE_SUIT, 3, 260, 260, 480, 480, 0}},
};

// 每组花色的原语数(其余槽位不参与渲染)。
static const uint8_t k_suit_counts[TAROT_EMBLEM_SUIT_COUNT] = {3, 3, 5, 2};

int tarot_suit_spec(int suit, const tarot_prim_spec_t **out) {
    if (out) *out = NULL;
    if (suit < 0 || suit >= TAROT_EMBLEM_SUIT_COUNT) return 0;
    if (out) *out = k_suit_specs[suit];
    return k_suit_counts[suit];
}

// ---------------------------------------------------------------------------
// 数字牌点阵(似扑克牌点;中心点归一化,数字区)。
// ---------------------------------------------------------------------------
static const uint16_t k_pip_xs[] = {
    500,
    500, 500,
    500, 500, 500,
    220, 780, 220, 780,
    220, 780, 500, 220, 780,
    220, 780, 220, 780, 220, 780,
    220, 780, 220, 780, 220, 780, 500,
    220, 780, 220, 780, 220, 780, 220, 780,
    220, 780, 220, 780, 220, 780, 220, 780, 500,
    220, 780, 220, 780, 220, 780, 220, 780, 500, 500,
};
static const uint16_t k_pip_ys[] = {
    500,
    150, 850,
    150, 500, 850,
    150, 150, 850, 850,
    150, 150, 500, 850, 850,
    150, 150, 500, 500, 850, 850,
    150, 150, 500, 500, 850, 850, 330,
    150, 150, 383, 383, 617, 617, 850, 850,
    150, 150, 383, 383, 617, 617, 850, 850, 500,
    150, 150, 383, 383, 617, 617, 850, 850, 266, 734,
};
// 下标 = rank 之前所有点数之和(1+2+...+(rank-1))。
static const uint16_t k_pip_off[11] = {0, 0, 1, 3,  6,  10,
                                       15, 21, 28, 36, 45};

int tarot_pip_layout(int rank, const uint16_t **xs, const uint16_t **ys) {
    if (xs) *xs = NULL;
    if (ys) *ys = NULL;
    if (rank < 1 || rank > 10) return 0;
    if (xs) *xs = &k_pip_xs[k_pip_off[rank]];
    if (ys) *ys = &k_pip_ys[k_pip_off[rank]];
    return rank;
}

// ---------------------------------------------------------------------------
// 宫廷牌布局:大符号盒 + 顶部徽记(侍从菱形/骑士三角/王后王冠/国王十字)。
// ---------------------------------------------------------------------------
bool tarot_court_layout(int rank, uint16_t *nx, uint16_t *ny,
                        uint16_t *nw, uint16_t *nh, int *insignia) {
    switch (rank) {
        case 11:  // 侍从
            if (nx) *nx = 200;
            if (ny) *ny = 300;
            if (nw) *nw = 600;
            if (nh) *nh = 600;
            if (insignia) *insignia = 2;
            return true;
        case 12:  // 骑士
            if (nx) *nx = 200;
            if (ny) *ny = 300;
            if (nw) *nw = 600;
            if (nh) *nh = 600;
            if (insignia) *insignia = 0;
            return true;
        case 13:  // 王后
            if (nx) *nx = 200;
            if (ny) *ny = 280;
            if (nw) *nw = 600;
            if (nh) *nh = 620;
            if (insignia) *insignia = 4;
            return true;
        case 14:  // 国王
            if (nx) *nx = 200;
            if (ny) *ny = 280;
            if (nw) *nw = 600;
            if (nh) *nh = 620;
            if (insignia) *insignia = 6;
            return true;
        default:
            return false;
    }
}
