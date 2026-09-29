<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

<h1 align="center">Tarot Divination</h1>

<p align="center">
  The complete Rider-Waite-Smith tarot — 78 cards, three keys, one pocket deck.<br>
  An application for the <a href="docs/README.md">FoloToy AI Passport</a> open wearable.
</p>

<p align="center">
  <img src="assets/images/home.jpg" alt="FoloToy AI Passport wearable device shown from the front, side, and back." width="100%">
</p>

## What it is

Tarot Divination turns the AI Passport into a standalone divination deck.
The firmware boots straight into the app — no menu, no demos — and carries the
full 78-card Rider-Waite-Smith deck (1909, public domain) with its classic
artwork embedded in flash.

- **78 complete cards** — 22 major arcana and 56 minor arcana, each with
  upright and reversed keywords and a short Chinese reading.
- **Single-card draw** — a shuffle animation, one card for the moment, then
  the reading page.
- **Three-card spread** — past, present and future, revealed one press at a
  time and browsed card by card.
- **True reversals** — about half the draws come up reversed: the artwork is
  rendered upside down with a cinnabar frame and a reversed badge.
- **Classic artwork** — all 78 scans compiled into the firmware at two
  resolutions (120 × 180 and 64 × 96), about 4.2 MB of the 8 MB flash.
- **Pocket-friendly power** — live battery percentage in the top-right corner.

## How to use

Three keys drive everything; every page returns to the cover with a long
press of OK.

| Page | UP / DOWN | OK (short press) | OK (long press) |
| --- | --- | --- | --- |
| Cover | choose single / three-card spread | start the draw | — |
| Shuffle | — | shuffle and reveal the next card | back to cover |
| Reading | scroll the reading text | cycle the three spread cards | back to cover |

## Install

Flash the verified merged image from `0x0` with
[esptool](https://docs.espressif.com/projects/esptool/) (any ESP32-C3
flash-tool equivalent works). A merged flash may reset stored data on the
device; see the [flashing and data policy](docs/development/engineering/firmware-layout.md).
The most recent validated build lives at
`build/FoloToy-AI-Passport-full.bin` after running the gate below.

## Build from source

Prerequisites: [ESP-IDF 5.5.3](docs/development/engineering/environment-setup.md)
on Windows, Linux or macOS.

```bash
./tools/validate.sh --static    # repository checks + host tests (no toolchain needed)
./tools/validate.sh --firmware  # ESP-IDF build + merged-image verification
./tools/validate.sh             # the complete gate
```

The gate produces `build/FoloToy-AI-Passport-full.bin` (merged, flash from
`0x0`) plus a hashed archive under `build/firmware/`. Details:
[build and test](docs/development/engineering/build-and-test.md).

## Regenerating assets

UI text, fonts and artwork are generated, deterministic, and committed:

```bash
python tools/gen_tarot_font_symbols.py   # glyph inventory from the sources
./tools/gen_tarot_fonts.sh               # Noto Sans SC subsets (needs node/npx)
python tools/gen_tarot_art.py --fetch    # card scans (public domain) + convert
python tools/gen_tarot_art.py --check    # verify committed art is up to date
```

Provenance and licenses are recorded in the
[assets documentation](assets/README.md).

## Development

- `main/` holds the whole application; `components/bsp` is the untouched
  board-support layer (display, buttons, audio, battery, shared I2C).
- Deck data, the draw engine, navigation and emblem geometry are pure C with
  no IDF/LVGL dependencies, covered by host tests inside the repository gate.
- AI-assisted work starts at [AGENTS.md](AGENTS.md); fork-specific workflow is
  described in the [fork guide](docs/fork-guide.md).
- The application archive with reusable engineering notes:
  [docs/reference/mutalisk999/tarot-divination](docs/reference/mutalisk999/tarot-divination/README.md).

## Hardware

| Item | Specification |
| --- | --- |
| Target | ESP32-C3, 8 MB flash, no PSRAM |
| Display | 240 × 320 portrait, ST7789P3 over SPI |
| Input | three keys (UP / DOWN / OK) on one ADC ladder |
| Extras used | battery gauge (CW2017) over shared I2C |

Board facts and the full capability contract:
[hardware guide](docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.md).

## Credits and license

- Card artwork: Rider-Waite-Smith tarot, 1909, by Pamela Colman Smith —
  public domain; scans via
  [searge/tarot](https://github.com/searge/tarot).
- Chinese font: Noto Sans SC, SIL Open Font License 1.1.
- Everything else: MIT — see [LICENSE](LICENSE). Contributions follow
  [CONTRIBUTING](.github/CONTRIBUTING.md).
