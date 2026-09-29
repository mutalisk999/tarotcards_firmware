// main/tarot_deck.h —— 塔罗牌库静态数据(78 张,纯逻辑,可主机测试,不依赖 IDF/LVGL)。
// 覆盖契约:设备上显示的全部中文都出自本仓库 main/tarot_*.c 的字符串常量,
// 字库子集由这些源文件提取生成(tools/gen_tarot_font_symbols.py)。
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAROT_CARD_COUNT 78
#define TAROT_MAJOR_COUNT 22
#define TAROT_MINOR_COUNT 56
#define TAROT_SUIT_NONE 0xFF

// 四花色(小阿卡纳)。顺序即牌 id 排布顺序:权杖 → 圣杯 → 宝剑 → 星币。
typedef enum {
    TAROT_SUIT_WANDS = 0,   // 权杖(火)
    TAROT_SUIT_CUPS,        // 圣杯(水)
    TAROT_SUIT_SWORDS,      // 宝剑(风)
    TAROT_SUIT_PENTACLES,   // 星币(土)
    TAROT_SUIT_COUNT
} tarot_suit_t;

// 一张牌。id 0..21 为大阿卡纳(suit = TAROT_SUIT_NONE,rank 即序号 0..21),
// id 22..77 为小阿卡纳(花色 × 数字 1..14,11..14 为宫廷牌)。
typedef struct {
    uint8_t id;             // 0..77
    uint8_t suit;           // tarot_suit_t 或 TAROT_SUIT_NONE
    uint8_t rank;           // 大阿卡纳 0..21;小阿卡纳 1..14
    const char *name_zh;    // 中文名,如 "愚者"、"权杖三"
    const char *keywords_up;    // 正位关键词,"·" 分隔
    const char *keywords_rev;   // 逆位关键词
    const char *meaning_up;     // 正位短解读(一两句)
    const char *meaning_rev;    // 逆位短解读
} tarot_card_t;

// 按牌 id 取牌;越界返回 NULL。
const tarot_card_t *tarot_card_get(int id);

// 是否大阿卡纳。id 越界返回 false。
bool tarot_card_is_major(int id);

// 花色中文名(权杖/圣杯/宝剑/星币);越界返回 NULL。
const char *tarot_suit_name(int suit);

// 花色元素中文名(火/水/风/土);越界返回 NULL。
const char *tarot_suit_element(int suit);

// 小阿卡纳数字牌位中文名:1..10 → 一..十,11..14 → 侍从/骑士/王后/国王;
// 越界返回 NULL。
const char *tarot_rank_name(int rank);

#ifdef __cplusplus
}
#endif
