// main/tarot_art.c —— 牌面艺术图访问器:正位直读 Flash,逆位行翻转进 RAM。
// 本板无 PSRAM,且 LVGL 的 transform 旋转会申请大块离屏层(分配失败会让
// 渲染线程空转假死),因此逆位不使用 lv_image_set_rotation,而是把图按行
// 倒序复制进静态缓冲(full 43.2KB + mini 12.3KB,.bss 合计约 55KB;
// 应用无网络协议栈,预算可容)。两套缓冲各自独立,允许牌面页(full)与
// 洗牌页(mini)同时持有一张逆位图。
#include "tarot_art.h"

#include <string.h>

static uint16_t s_reversed_buf[TAROT_ART_W * TAROT_ART_H];
static uint16_t s_reversed_mini_buf[TAROT_ART_MINI_W * TAROT_ART_MINI_H];

static const lv_image_dsc_t s_reversed_dsc = {
    .header.magic = LV_IMAGE_HEADER_MAGIC,
    .header.cf = LV_COLOR_FORMAT_RGB565,
    .header.w = TAROT_ART_W,
    .header.h = TAROT_ART_H,
    .header.stride = TAROT_ART_W * 2,
    .data = (const uint8_t *)s_reversed_buf,
    .data_size = sizeof(s_reversed_buf),
};

static const lv_image_dsc_t s_reversed_mini_dsc = {
    .header.magic = LV_IMAGE_HEADER_MAGIC,
    .header.cf = LV_COLOR_FORMAT_RGB565,
    .header.w = TAROT_ART_MINI_W,
    .header.h = TAROT_ART_MINI_H,
    .header.stride = TAROT_ART_MINI_W * 2,
    .data = (const uint8_t *)s_reversed_mini_buf,
    .data_size = sizeof(s_reversed_mini_buf),
};

// 把 upright 的像素按行倒序复制进 dst(w×h)。
static void flip_into(const lv_image_dsc_t *upright, uint16_t *dst,
                      int w, int h) {
    const uint16_t *src = (const uint16_t *)upright->data;
    for (int row = 0; row < h; row++) {
        memcpy(&dst[(size_t)row * w],
               &src[(size_t)(h - 1 - row) * w], (size_t)w * sizeof(uint16_t));
    }
}

const lv_image_dsc_t *tarot_art_get(int card_id, bool reversed) {
    if (card_id < 0 || card_id >= TAROT_ART_COUNT) return NULL;
    const lv_image_dsc_t *upright = &tarot_card_images[card_id];
    if (!reversed) return upright;
    flip_into(upright, s_reversed_buf, TAROT_ART_W, TAROT_ART_H);
    return &s_reversed_dsc;
}

const lv_image_dsc_t *tarot_art_mini_get(int card_id, bool reversed) {
    if (card_id < 0 || card_id >= TAROT_ART_COUNT) return NULL;
    const lv_image_dsc_t *upright = &tarot_card_mini_images[card_id];
    if (!reversed) return upright;
    flip_into(upright, s_reversed_mini_buf, TAROT_ART_MINI_W, TAROT_ART_MINI_H);
    return &s_reversed_mini_dsc;
}
