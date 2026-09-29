// main/tarot_nav.h —— 塔罗应用导航与交互决策(纯逻辑,可主机测试,不依赖 IDF/LVGL)。
// 页面流:首页 → 抽牌 → 牌面/解读;OK 长按统一回首页。
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 应用页面状态(自设计导航流,不复用 baseline demo 菜单)。
typedef enum {
    TAROT_PAGE_HOME = 0,   // 封面:选择牌阵
    TAROT_PAGE_CAST,       // 洗牌:动画 + 逐张定牌
    TAROT_PAGE_CARD,       // 牌面:符号牌面 + 关键词 + 解读
    TAROT_PAGE_COUNT,
} tarot_page_t;

// 牌阵选择(首页两个选项)。
typedef enum {
    TAROT_MODE_SINGLE = 0,  // 单张抽牌
    TAROT_MODE_THREE,       // 三牌阵(过去/现在/未来)
    TAROT_MODE_COUNT,
} tarot_mode_t;

// 模式循环步进:UP/DOWN 在两个模式间循环;delta 任意正负,越界返回 -1。
int tarot_mode_step(int mode, int delta);

// 模式对应的抽牌张数;mode 越界返回 0。
int tarot_mode_draw_count(int mode);

// 牌面页浏览步进:在 count 张牌间循环;count<=0 或 index 越界返回 -1。
int tarot_card_step(int index, int count, int delta);

// OK 长按的返回目标:抽牌页/牌面页回首页,首页留在首页。
tarot_page_t tarot_nav_back_target(tarot_page_t page);

// 页面流转合法性:牌面页必须已有已抽好的牌阵(cast_done),否则视为非法流转。
bool tarot_nav_can_enter(tarot_page_t page, bool cast_done);

#ifdef __cplusplus
}
#endif
