// main/tarot_engine.c —— 塔罗抽牌引擎实现。
#include "tarot_engine.h"

#include "tarot_deck.h"

// xorshift32:状态不得为 0(全零是死态)。种子为 0 时映射到固定非零初值,
// 保证"种子 0"也是合法且确定的输入。
uint32_t tarot_rand_next(uint32_t *state) {
    uint32_t x = *state;
    if (x == 0) x = 0x9E3779B9u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

const char *tarot_spread_position_label(int index) {
    static const char *const k_labels[3] = {"过去", "现在", "未来"};
    if (index < 0 || index > 2) return NULL;
    return k_labels[index];
}

bool tarot_draw_spread(uint8_t count, uint32_t seed, tarot_spread_t *out) {
    if (out == NULL) return false;
    if (count != TAROT_SPREAD_SINGLE && count != TAROT_SPREAD_THREE) return false;

    // Fisher-Yates 部分洗牌:78 张编号依次就位,每张抽牌从未选区随机换一张上来。
    // 同一副牌阵天然不重复;种子确定则整副牌阵确定。
    uint8_t deck[TAROT_CARD_COUNT];
    for (int i = 0; i < TAROT_CARD_COUNT; i++) deck[i] = (uint8_t)i;

    uint32_t rand = seed;
    for (int i = 0; i < count; i++) {
        int pick = i + (int)(tarot_rand_next(&rand) % (uint32_t)(TAROT_CARD_COUNT - i));
        uint8_t tmp = deck[i];
        deck[i] = deck[pick];
        deck[pick] = tmp;
        out->draws[i].card = deck[i];
        out->draws[i].reversed = (tarot_rand_next(&rand) & 1u) != 0;
    }
    out->count = count;
    return true;
}
