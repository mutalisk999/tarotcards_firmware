// main/tarot_emblem.h —— 塔罗牌面符号渲染(几何原语 → LVGL 部件,无位图)。
// 布局与镜像的数学在 tarot_emblem_math.c(纯逻辑,主机可测);
// 本模块只做"规格 → 部件",并处理 LVGL 角度/点表的方向约定。
#pragma once

#include <stdbool.h>

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// 在 parent 的 (x,y,w,h) 区域绘制一张牌面:
// 边框 + 牌面符号。card_id 0..77;reversed=true 时符号整体上下颠倒。
// 区域过小(宽或高 < 40px)时只画边框与花色色块,保证可辨。
lv_obj_t *tarot_emblem_create(lv_obj_t *parent, int card_id, bool reversed,
                              int x, int y, int w, int h);

#ifdef __cplusplus
}
#endif
