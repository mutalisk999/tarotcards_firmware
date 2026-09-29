<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Tarot Divination

A full 78-card tarot reader that turns the AI Passport into a pocket divination
deck. Draw one card for the moment, or lay out past / present / future, with the
complete public-domain Rider-Waite-Smith (1909) artwork on a 2.4-inch screen.

## Publish information

- **Title**: Tarot Divination
- **Description**: Carry the complete Rider-Waite-Smith tarot in your pocket.
  Shuffle with a key press, draw a single card for the day, or lay out a
  three-card spread for past, present and future. Every card shows the classic
  1909 artwork, upright or reversed, with Chinese keywords and a short reading.
- **App name (kebab-case)**: `tarot-divination`

## What it does

- **Boots straight into the app**: the firmware opens the cover page directly
  with no main menu; the baseline hardware-test demo UI is not used.
- **Complete 78-card deck**: 22 major arcana plus 56 minor arcana, each with
  upright and reversed keywords and a one-paragraph Chinese reading following
  the common Rider-Waite meanings.
- **Embedded artwork**: all 78 classic card scans (public domain, 1909) are
  compiled into the firmware at two resolutions — 120 × 180 for the reading
  page and single-card reveal, 64 × 96 for three-card reveal slots (about
  4.2 MB of the 8 MB flash; no images are scaled at runtime).
- **Single-card draw**: a shuffle animation plays, the hardware RNG settles
  the whole spread at the first press, and the card is revealed with its art;
  the app then moves to the reading page automatically.
- **Three-card spread**: past / present / future, revealed one press at a
  time, browsed card by card afterwards.
- **Reversals**: each card is upright or reversed at about 50/50. A reversed
  card renders the artwork truly upside down (row-flipped into a static RAM
  buffer, since this board cannot transform images without an offscreen
  layer), with a cinnabar frame and a reversed badge.
- **Battery readout**: live percentage in the top-right corner, refreshed
  every few seconds; below 20 % it turns cinnabar.

## Interaction

Three keys drive the whole app.

- **Cover page**: UP/DOWN select the spread (single / three-card), OK starts
  it.
- **Shuffle page**: OK runs the shuffle and reveals the next card; after the
  last card, OK opens the reading page; OK (long press) returns to the cover.
- **Reading page**: UP/DOWN scroll the reading text; in a three-card spread OK
  cycles past → present → future; OK (long press) returns to the cover.

## Engineering notes worth reusing

- **One image set per display size**: LVGL cannot scale images on this
  no-PSRAM board (transforms need an offscreen layer), so the generator emits
  a 1:1 set for each slot size instead.
- **Reversed art without transforms**: upside-down display is a row-reversed
  copy into static RAM buffers (43 KB full + 12 KB mini), not `lv_image`
  rotation.
- **Font subset as a contract**: the UI text inventory is extracted from the
  sources and the Noto Sans SC subsets are regenerated from it; a host test
  fails the gate when text and inventory drift apart.
- **Pure logic kept testable**: deck data, draw engine (seeded xorshift32 +
  partial Fisher-Yates), navigation, and emblem geometry are IDF/LVGL-free and
  covered by host tests in the repository gate.

## Source

- Repository: `mutalisk999/tarotcards_firmware`, branch
  `feature/tarot-divination`
  (<https://github.com/mutalisk999/tarotcards_firmware/tree/feature/tarot-divination>).
- Firmware: `build/FoloToy-AI-Passport-full.bin`, flashed from `0x0`
  (merged image about 5.5 MB of the 8 MB flash).
