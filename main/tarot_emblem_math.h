// main/tarot_emblem_math.h —— 塔罗牌面符号几何(纯逻辑,可主机测试,不依赖 IDF/LVGL)。
//
// 本板无 PSRAM,LVGL 的 transform 类样式会产生离屏层(分配失败会让渲染线程
// 空转假死,见 liuyao/theme 同款教训),因此逆位牌面不做图形旋转:
// 一切符号都由"轴对齐矩形/圆环/弧/折线轮廓"组成,逆位 = 归一化坐标在
// y 方向镜像(纯函数,主机可测)。
//
// 坐标约定:所有归一化坐标取 0..TAROT_EMBLEM_NORM(含端点),相对符号自身的
// 包围盒;放置时线性映射到像素区域,先算 x 后镜像 y。
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAROT_EMBLEM_NORM 1000
#define TAROT_EMBLEM_SUIT_COUNT 4

// 可渲染的符号原语(全部可用无离屏层的 LVGL 部件表达)。
typedef enum {
    TAROT_PRIM_BAR = 0,   // 实心矩形(nx,ny,nw,nh 即盒子)
    TAROT_PRIM_DOT,       // 实心圆(盒内最大内切圆)
    TAROT_PRIM_RING,      // 空心圆环(描边圆)
    TAROT_PRIM_ARC,       // 弧段(aux/aux2 = 起/止角度,0 度=3 点钟,顺时针)
    TAROT_PRIM_POLY,      // 闭合折线轮廓(aux = 轮廓点表下标,铺满盒子)
    TAROT_PRIM_KIND_COUNT
} tarot_prim_kind_t;

// 符号配色角色(theme 层映射为实际颜色)。
typedef enum {
    TAROT_ROLE_GOLD = 0,   // 鎏金:主线与主体
    TAROT_ROLE_PAPER,      // 宣纸白:次级结构
    TAROT_ROLE_DIM,        // 暗金:装饰
    TAROT_ROLE_SUIT,       // 花色色(小阿卡纳)
    TAROT_ROLE_ACCENT,     // 朱砂:强调点
    TAROT_ROLE_BG,         // 页面底色:遮挡用(如月牙的补形圆)
    TAROT_ROLE_COUNT
} tarot_prim_role_t;

// 一个符号原语的规格。坐标/尺寸为归一化值(0..1000)。
typedef struct {
    uint8_t kind;    // tarot_prim_kind_t
    uint8_t role;    // tarot_prim_role_t
    uint16_t aux;    // POLY: 轮廓表下标;ARC: 起始角(度,0=3 点钟顺时针)
    uint16_t nx, ny, nw, nh;
    uint16_t aux2;   // ARC: 结束角(度)
} tarot_prim_spec_t;

// 把规格映射为像素矩形。w/h 为符号区域像素尺寸;reversed 时做 y 镜像。
// ARC/POLY 的盒子同样镜像,点表由渲染层在镜像后的盒内换算。
void tarot_prim_place(const tarot_prim_spec_t *spec, int w, int h,
                      bool reversed, int *out_x, int *out_y,
                      int *out_w, int *out_h);

// 轮廓点表:返回点数并把归一化点数组指针写出(xs/ys 各 count 个,
// 取值 0..1000,相对 POLY 自身盒子)。index 非法返回 0。
int tarot_poly_points(int index, const uint16_t **xs, const uint16_t **ys);

// —— 大阿卡纳专属符号(22 组构图) ——
// 返回原语数量并把规格数组指针写出;id 须为 0..21,越界返回 0。
int tarot_major_spec(int id, const tarot_prim_spec_t **out);

// —— 小阿卡纳花色符号(4 组构图,任意尺寸复用) ——
// suit 须为 0..3,越界返回 0。数字牌的点阵与宫廷牌都复用同一组花色符号。
int tarot_suit_spec(int suit, const tarot_prim_spec_t **out);

// —— 数字牌点阵布局(似扑克牌点) ——
// rank 1..10;把 rank 个花色符号的归一化中心点写出(xs/ys 各 rank 个)。
// rank 非法返回 0。点坐标为 0..1000,相对数字区。
int tarot_pip_layout(int rank, const uint16_t **xs, const uint16_t **ys);

// —— 宫廷牌布局 ——
// rank 11..14;花色符号占大盒(nx,ny,nw,nh),徽记点表由 insignia 给出
// (POLY 下标,-1 表示无徽记)。rank 越界返回 false。
bool tarot_court_layout(int rank, uint16_t *nx, uint16_t *ny,
                        uint16_t *nw, uint16_t *nh, int *insignia);

#ifdef __cplusplus
}
#endif
