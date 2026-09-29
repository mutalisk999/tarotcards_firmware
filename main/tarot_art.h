// main/tarot_art.h —— 嵌入式牌面艺术图访问器(RWS 1909 公有领域扫描)。
// 两套分辨率(由 tools/gen_tarot_art.py 生成于 assets/images/):
//   full 120×180:牌面解读页与单张洗牌揭示;
//   mini  64×96 :三牌阵洗牌揭示的小卡位。
#pragma once

#include <stdbool.h>

// 生成的图集声明(描述符数组与 TAROT_ART_* 尺寸宏)。
// 用相对路径包含:与本文件同名,裸文件名会被本目录的同名头抢先解析。
#include "../assets/images/tarot_art.h"

#ifdef __cplusplus
extern "C" {
#endif

// 取 full 套图。card_id 0..77;reversed=true 返回上下颠倒的副本
// (渲染进静态 RAM 缓冲,同一时刻只保证"当前显示的一张"有效)。
const lv_image_dsc_t *tarot_art_get(int card_id, bool reversed);

// 取 mini 套图(64×96),语义同上;逆位使用独立的静态缓冲。
const lv_image_dsc_t *tarot_art_mini_get(int card_id, bool reversed);

#ifdef __cplusplus
}
#endif
