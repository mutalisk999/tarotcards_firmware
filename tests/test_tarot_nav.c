// tests/test_tarot_nav.c —— 导航与交互决策主机测试。
#include <assert.h>
#include <stdio.h>

#include "tarot_nav.h"

int main(void) {
    // 模式循环。
    assert(tarot_mode_step(TAROT_MODE_SINGLE, 1) == TAROT_MODE_THREE);
    assert(tarot_mode_step(TAROT_MODE_THREE, 1) == TAROT_MODE_SINGLE);
    assert(tarot_mode_step(TAROT_MODE_SINGLE, -1) == TAROT_MODE_THREE);
    assert(tarot_mode_step(TAROT_MODE_THREE, 5) == TAROT_MODE_SINGLE);
    assert(tarot_mode_step(TAROT_MODE_SINGLE, -3) == TAROT_MODE_THREE);
    assert(tarot_mode_step(-1, 1) == -1);
    assert(tarot_mode_step(TAROT_MODE_COUNT, 1) == -1);

    // 模式 → 张数。
    assert(tarot_mode_draw_count(TAROT_MODE_SINGLE) == 1);
    assert(tarot_mode_draw_count(TAROT_MODE_THREE) == 3);
    assert(tarot_mode_draw_count(2) == 0);
    assert(tarot_mode_draw_count(-1) == 0);

    // 牌面浏览循环。
    assert(tarot_card_step(0, 3, 1) == 1);
    assert(tarot_card_step(2, 3, 1) == 0);
    assert(tarot_card_step(0, 3, -1) == 2);
    assert(tarot_card_step(0, 1, 7) == 0);
    assert(tarot_card_step(0, 0, 1) == -1);
    assert(tarot_card_step(3, 3, 1) == -1);
    assert(tarot_card_step(-1, 3, 1) == -1);

    // OK 长按一律回首页。
    for (int page = 0; page < TAROT_PAGE_COUNT; page++) {
        assert(tarot_nav_back_target((tarot_page_t)page) == TAROT_PAGE_HOME);
    }

    // 牌面页必须先抽完牌。
    assert(tarot_nav_can_enter(TAROT_PAGE_HOME, false));
    assert(tarot_nav_can_enter(TAROT_PAGE_CAST, false));
    assert(!tarot_nav_can_enter(TAROT_PAGE_CARD, false));
    assert(tarot_nav_can_enter(TAROT_PAGE_CARD, true));
    assert(!tarot_nav_can_enter((tarot_page_t)99, true));

    printf("nav: modes/steps/back/can-enter PASS\n");
    return 0;
}
