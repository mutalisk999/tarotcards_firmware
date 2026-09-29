// main/tarot_engine.h —— 塔罗抽牌引擎(纯逻辑,可主机测试,不依赖 IDF/LVGL)。
// 牌阵规模固定两种:单张与三牌阵(过去/现在/未来)。
// 随机源由调用方注入种子:设备用 esp_random(),主机测试用固定种子,
// 同一种子序列完全确定,便于黄金向量式断言。
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAROT_DRAW_MAX 3       // 单张 = 1,三牌阵 = 3
#define TAROT_SPREAD_SINGLE 1
#define TAROT_SPREAD_THREE 3

// 牌阵位置含义(仅三牌阵使用)。
const char *tarot_spread_position_label(int index);  // 过去/现在/未来

// 一次抽牌:牌 id(0..77)+ 正逆位。
typedef struct {
    uint8_t card;      // 0..77,与 tarot_deck.h 的 id 对应
    bool reversed;     // true = 逆位
} tarot_draw_t;

typedef struct {
    uint8_t count;                  // 1 或 3
    tarot_draw_t draws[TAROT_DRAW_MAX];
} tarot_spread_t;

// 抽一副牌阵。count 必须是 1 或 3,out 为 NULL 或 count 非法返回 false。
// 同一副牌阵内牌不重复;每张正逆位各约 50%。
bool tarot_draw_spread(uint8_t count, uint32_t seed, tarot_spread_t *out);

// 引擎内部用的确定性伪随机流(xorshift32)。
// 独立导出:一是测试序列分布,二是页面动画可以直接复用同一算法。
uint32_t tarot_rand_next(uint32_t *state);

#ifdef __cplusplus
}
#endif
