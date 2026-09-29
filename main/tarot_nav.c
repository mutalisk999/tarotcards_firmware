// main/tarot_nav.c —— 塔罗应用导航与交互决策实现。
#include "tarot_nav.h"

#include "tarot_engine.h"

int tarot_mode_step(int mode, int delta) {
    if (mode < 0 || mode >= TAROT_MODE_COUNT) return -1;
    int next = mode + delta;
    // 循环步进:delta 可能大于 1(连续快按),取模前先归一。
    next %= TAROT_MODE_COUNT;
    if (next < 0) next += TAROT_MODE_COUNT;
    return next;
}

int tarot_mode_draw_count(int mode) {
    if (mode == TAROT_MODE_SINGLE) return TAROT_SPREAD_SINGLE;
    if (mode == TAROT_MODE_THREE) return TAROT_SPREAD_THREE;
    return 0;
}

int tarot_card_step(int index, int count, int delta) {
    if (count <= 0 || index < 0 || index >= count) return -1;
    int next = (index + delta) % count;
    if (next < 0) next += count;
    return next;
}

tarot_page_t tarot_nav_back_target(tarot_page_t page) {
    // 本应用只有三层:任何页面的 OK 长按都回首页(首页长按原地不动)。
    (void)page;
    return TAROT_PAGE_HOME;
}

bool tarot_nav_can_enter(tarot_page_t page, bool cast_done) {
    if (page < 0 || page >= TAROT_PAGE_COUNT) return false;
    // 牌面页依赖已抽好的牌阵;首页与抽牌页随时可进。
    if (page == TAROT_PAGE_CARD) return cast_done;
    return true;
}
