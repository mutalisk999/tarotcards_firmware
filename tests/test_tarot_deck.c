// tests/test_tarot_deck.c —— 牌库数据完整性主机测试。
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "tarot_deck.h"

int main(void) {
    // 越界与边界。
    assert(tarot_card_get(-1) == NULL);
    assert(tarot_card_get(0) != NULL);
    assert(tarot_card_get(TAROT_CARD_COUNT - 1) != NULL);
    assert(tarot_card_get(TAROT_CARD_COUNT) == NULL);

    int major_count = 0;
    for (int id = 0; id < TAROT_CARD_COUNT; id++) {
        const tarot_card_t *c = tarot_card_get(id);
        assert(c != NULL);
        assert(c->id == id);
        // 四段文案全部非空,长度留有版面余量(UTF-8 中文每字 3 字节)。
        assert(c->name_zh && c->name_zh[0]);
        assert(strlen(c->name_zh) <= 21);
        assert(c->keywords_up && c->keywords_up[0]);
        assert(strlen(c->keywords_up) <= 60);
        assert(c->keywords_rev && c->keywords_rev[0]);
        assert(strlen(c->keywords_rev) <= 60);
        assert(c->meaning_up && c->meaning_up[0]);
        assert(strlen(c->meaning_up) <= 180);
        assert(c->meaning_rev && c->meaning_rev[0]);
        assert(strlen(c->meaning_rev) <= 180);

        if (tarot_card_is_major(id)) {
            major_count++;
            assert(id < TAROT_MAJOR_COUNT);
            assert(c->suit == TAROT_SUIT_NONE);
            assert(c->rank == (uint8_t)id);
        } else {
            int rel = id - TAROT_MAJOR_COUNT;
            assert(rel >= 0 && rel < TAROT_MINOR_COUNT);
            assert(c->suit == (uint8_t)(rel / 14));
            assert(c->rank == (uint8_t)(rel % 14 + 1));
        }
    }
    assert(major_count == TAROT_MAJOR_COUNT);

    // 花色与数字牌位名称。
    for (int suit = 0; suit < TAROT_SUIT_COUNT; suit++) {
        assert(tarot_suit_name(suit) != NULL);
        assert(tarot_suit_element(suit) != NULL);
    }
    assert(tarot_suit_name(TAROT_SUIT_COUNT) == NULL);
    assert(tarot_suit_element(-1) == NULL);
    for (int rank = 1; rank <= 14; rank++) {
        assert(tarot_rank_name(rank) != NULL);
    }
    assert(tarot_rank_name(0) == NULL);
    assert(tarot_rank_name(15) == NULL);

    assert(tarot_card_is_major(TAROT_MAJOR_COUNT - 1));
    assert(!tarot_card_is_major(TAROT_MAJOR_COUNT));
    assert(!tarot_card_is_major(-1));

    // 抽查:标准顺序的两端。
    assert(strcmp(tarot_card_get(0)->name_zh, "愚者") == 0);
    assert(strcmp(tarot_card_get(21)->name_zh, "世界") == 0);
    assert(strcmp(tarot_card_get(22)->name_zh, "权杖一") == 0);
    assert(strcmp(tarot_card_get(35)->name_zh, "权杖国王") == 0);
    assert(strcmp(tarot_card_get(77)->name_zh, "星币国王") == 0);

    printf("deck: %d cards PASS\n", TAROT_CARD_COUNT);
    return 0;
}
