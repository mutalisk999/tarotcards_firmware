#!/usr/bin/env python3
"""Extract the Chinese/Latin character inventory of the tarot firmware UI.

Scans every C string literal in the tarot application sources (main/*.c that
carry UI or engine text) and writes the unique character set to
assets/fonts/tarot-symbols.txt. That inventory is the font-subset contract:
regenerate fonts (tools/gen_tarot_fonts.sh) whenever it changes.

    python tools/gen_tarot_font_symbols.py
"""

from __future__ import annotations

import re
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
SOURCES = [
    "main/main.c",
    "main/tarot_app.c",
    "main/tarot_deck.c",
    "main/tarot_engine.c",
    "main/tarot_nav.c",
    "main/tarot_emblem_math.c",
    "main/tarot_theme.c",
    "main/tarot_emblem.c",
    "main/tarot_pages_home.c",
    "main/tarot_pages_cast.c",
    "main/tarot_pages_card.c",
]

# 48px 封面大字只用于这几个字(独立小字库);16/24px 用全量清单。
COVER_SUBSET = "塔罗"

STRING_RE = re.compile(r'"((?:[^"\\\n]|\\.)*)"')


def collect() -> str:
    chars: set[str] = set()
    for rel in SOURCES:
        path = REPO / rel
        if not path.exists():
            raise SystemExit(f"missing source: {rel}")
        text = path.read_text(encoding="utf-8")
        for literal in STRING_RE.findall(text):
            chars.update(literal)
    # 基础保障:可打印 ASCII、常用全角标点与特殊符号。
    # 标点包含若干当前文案未使用的字形(？！…书名号等):刻意保留的排版
    # 储备,避免日后改文案还要重生成字库。正文统一用全角逗号/句号,
    # 冒号与括号用半角,因此储备里不含全角冒号与全角括号。
    chars.update(chr(c) for c in range(0x20, 0x7F))
    chars.update("，。、；：？！·—…《》「」()％‰℃")
    chars.update("○×✕")  # 装饰符号
    chars.discard("\n")
    chars.discard("\t")
    chars.discard("\\")
    return "".join(sorted(chars))


def main() -> None:
    all_chars = collect()
    out = REPO / "assets/fonts"
    out.mkdir(parents=True, exist_ok=True)
    (out / "tarot-symbols.txt").write_text(all_chars, encoding="utf-8")
    (out / "tarot-symbols-cover.txt").write_text(
        "".join(sorted(set(COVER_SUBSET))), encoding="utf-8")
    cjk = sum(1 for c in all_chars if ord(c) > 0x2E7F)
    print(f"inventory: {len(all_chars)} chars ({cjk} CJK) -> assets/fonts/")


if __name__ == "__main__":
    main()
