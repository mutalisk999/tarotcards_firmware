// main/tarot_theme.h —— 塔罗应用视觉主题:配色、字体、公共组件。
// 独立于 baseline demo 的 ui_pixel 外壳(衍生应用强制 UI 重设计)。
#pragma once

#include "lvgl.h"

// 星夜 + 羊皮纸 + 鎏金 + 朱砂的神秘学配色。
#define TT_COLOR_BG        0x16121F  // 星夜底(页面上部)
#define TT_COLOR_BG_2      0x100D18  // 星夜底(页面下部,纵向渐变)
#define TT_COLOR_PANEL     0x221C30  // 面板
#define TT_COLOR_PANEL_2   0x2E2740  // 面板高亮
#define TT_COLOR_TINT      0x2E1A2B  // 逆位暗晕底色
#define TT_COLOR_PAPER     0xEFE6D5  // 羊皮纸字色
#define TT_COLOR_INK       0x241F31  // 墨字(浅底上的正文字色)
#define TT_COLOR_PAPER_DIM 0x9C92AA  // 次要字色
#define TT_COLOR_GOLD      0xD9A441  // 鎏金
#define TT_COLOR_GOLD_DIM  0x8A6D2F  // 暗金(角饰、装饰线)
#define TT_COLOR_CINNABAR  0xC03A2B  // 朱砂(逆位、强调)
// 四花色色:权杖之火 / 圣杯之水 / 宝剑之风 / 星币之土。
#define TT_COLOR_SUIT_WANDS     0xD96C3F
#define TT_COLOR_SUIT_CUPS      0x5B8FD9
#define TT_COLOR_SUIT_SWORDS    0xA9B4C9
#define TT_COLOR_SUIT_PENTACLES 0x8FA05E

// 应用字库(assets/fonts 生成,见 tools/gen_tarot_fonts.sh)。
LV_FONT_DECLARE(tarot_font_16);
LV_FONT_DECLARE(tarot_font_24);
LV_FONT_DECLARE(tarot_font_48);

// 页面脚手架:深底 + 标题 + 四角角饰。返回页面根对象(全屏)。
lv_obj_t *tarot_page_create(const char *title);

// 右上角电量组件刷新;soc<0 时显示 "--",低电量转朱砂色。
void tarot_battery_update(lv_obj_t *battery_label, int soc);

// —— 通用装饰组件(均为轴对齐矩形/圆/弧/折线,不产生离屏层) ——

// 纯色矩形(去默认样式,圆角 2)。
lv_obj_t *tarot_rect_create(lv_obj_t *parent, int x, int y, int w, int h,
                            uint32_t color);

// 空心圆环(描边圆)。
lv_obj_t *tarot_ring_create(lv_obj_t *parent, int cx, int cy, int radius,
                            int width, uint32_t color, lv_opa_t opa);

// 实心圆。
lv_obj_t *tarot_circle_create(lv_obj_t *parent, int cx, int cy, int radius,
                              uint32_t color);

// 圆弧段(角度为 LVGL 约定:0 度 = 3 点钟,顺时针;线宽 width)。
lv_obj_t *tarot_arc_create(lv_obj_t *parent, int cx, int cy, int radius,
                           int width, uint32_t color, int start_angle,
                           int end_angle);

// 闭合折线轮廓(点数组为像素坐标;内部复制到静态池,返回后可释放原数组)。
lv_obj_t *tarot_poly_create(lv_obj_t *parent, const lv_point_precise_t *points,
                            int count, uint32_t color, int line_width);

// 复位折线点池(在旧屏幕已删除、新牌面构建前调用)。
void tarot_theme_reset_poly_pool(void);

// 水平细分隔线(1px,面板高亮色)。
lv_obj_t *tarot_hairline(lv_obj_t *parent, int x, int y, int w);

// 页面四角暗金角饰(低透明度,营造牌桌边框氛围)。
void tarot_corner_ornaments(lv_obj_t *parent);

// 底部操作提示行:统一字体/颜色,水平居中于 y。
lv_obj_t *tarot_hint_create(lv_obj_t *parent, int y, const char *text);
