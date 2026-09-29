// tests/test_tarot_engine.c —— 抽牌引擎主机测试:确定性、不重复、分布、覆盖。
#include <assert.h>
#include <stdio.h>

#include "tarot_deck.h"
#include "tarot_engine.h"

int main(void) {
    // 非法输入。
    assert(!tarot_draw_spread(0, 1, NULL));
    tarot_spread_t sp;
    assert(!tarot_draw_spread(0, 1, &sp));
    assert(!tarot_draw_spread(2, 1, &sp));
    assert(!tarot_draw_spread(4, 1, &sp));
    assert(!tarot_draw_spread(TAROT_SPREAD_SINGLE, 1, NULL));

    // 同种子完全确定。
    tarot_spread_t a, b;
    assert(tarot_draw_spread(TAROT_SPREAD_SINGLE, 42, &a));
    assert(tarot_draw_spread(TAROT_SPREAD_SINGLE, 42, &b));
    assert(a.count == 1 && b.count == 1);
    assert(a.draws[0].card == b.draws[0].card);
    assert(a.draws[0].reversed == b.draws[0].reversed);

    // 种子 0 是合法且确定的输入(内部映射为非零初值)。
    assert(tarot_draw_spread(TAROT_SPREAD_SINGLE, 0, &a));
    assert(tarot_draw_spread(TAROT_SPREAD_SINGLE, 0, &b));
    assert(a.draws[0].card == b.draws[0].card);

    // 不同种子给出不同结果(确定性引擎下,这两个种子必然分流)。
    assert(tarot_draw_spread(TAROT_SPREAD_SINGLE, 1, &a));
    assert(tarot_draw_spread(TAROT_SPREAD_SINGLE, 2, &b));
    assert(a.draws[0].card != b.draws[0].card ||
           a.draws[0].reversed != b.draws[0].reversed);

    // 三牌阵:张数正确、三张互不重复、牌号都在 0..77。
    assert(tarot_draw_spread(TAROT_SPREAD_THREE, 7, &a));
    assert(a.count == 3);
    for (int i = 0; i < 3; i++) {
        assert(a.draws[i].card < TAROT_CARD_COUNT);  // card 是 uint8_t,防截断
        for (int j = i + 1; j < 3; j++) {
            assert(a.draws[i].card != a.draws[j].card);
        }
    }

    // xorshift32:零状态不锁死,输出非零。
    uint32_t st = 0;
    assert(tarot_rand_next(&st) != 0);

    // 分布与覆盖:4000 次单抽下,每张牌都该出现,正逆位各接近一半。
    int seen[TAROT_CARD_COUNT] = {0};
    int reversed_count = 0;
    const int k_runs = 4000;
    for (int i = 0; i < k_runs; i++) {
        tarot_spread_t s;
        assert(tarot_draw_spread(TAROT_SPREAD_SINGLE, (uint32_t)(i * 2654435761u + 1), &s));
        assert(s.draws[0].card < TAROT_CARD_COUNT);
        seen[s.draws[0].card]++;
        if (s.draws[0].reversed) reversed_count++;
    }
    for (int id = 0; id < TAROT_CARD_COUNT; id++) {
        if (seen[id] == 0) {
            fprintf(stderr, "card %d never drawn in %d runs\n", id, k_runs);
            return 1;
        }
    }
    double rev_ratio = (double)reversed_count / k_runs;
    assert(rev_ratio > 0.45 && rev_ratio < 0.55);

    printf("engine: determinism + coverage(%d runs) PASS\n", k_runs);
    return 0;
}
