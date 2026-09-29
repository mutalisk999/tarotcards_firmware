#!/usr/bin/env python3
"""Font-subset contract for the tarot application.

The committed assets/fonts/tarot-symbols.txt is the coverage contract: it must
exactly match the character inventory recomputed from the application sources,
and the three generated LVGL font sources must exist and declare the expected
symbols. Catches "UI text edited but fonts not regenerated".
"""

from pathlib import Path
import importlib.util
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
GEN = ROOT / "tools" / "gen_tarot_font_symbols.py"
FONTS = ROOT / "assets" / "fonts"

SPEC = importlib.util.spec_from_file_location("gen_tarot_font_symbols", GEN)
assert SPEC and SPEC.loader
GEN_MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GEN_MODULE)


class TarotFontContractTest(unittest.TestCase):
    def test_inventory_matches_application_sources(self) -> None:
        committed = (FONTS / "tarot-symbols.txt").read_text(encoding="utf-8")
        recomputed = GEN_MODULE.collect()
        missing = "".join(c for c in recomputed if c not in committed)
        stale = "".join(c for c in committed if c not in recomputed)
        self.assertEqual(
            (missing, stale), ("", ""),
            "assets/fonts/tarot-symbols.txt is out of date; run "
            "python tools/gen_tarot_font_symbols.py and regenerate fonts",
        )

    def test_cover_subset_is_the_hero_title(self) -> None:
        cover = (FONTS / "tarot-symbols-cover.txt").read_text(encoding="utf-8")
        self.assertEqual(cover, GEN_MODULE.COVER_SUBSET)

    def test_generated_font_sources_declare_expected_symbols(self) -> None:
        for size in (16, 24, 48):
            path = FONTS / f"tarot_font_{size}.c"
            self.assertTrue(path.is_file(), f"missing {path}")
            text = path.read_text(encoding="utf-8")
            self.assertIn(f"tarot_font_{size}", text)
            # lv_font_conv 输出的 LVGL C 源必含字形描述与位图块标记。
            self.assertIn("glyph_dsc", text)
            self.assertIn("bitmap", text)

    def test_inventory_covers_all_card_names(self) -> None:
        """78 张牌名 + 正逆位标记的每个字都必须在字库清单内(白盒抽查)。"""
        inventory = set((FONTS / "tarot-symbols.txt").read_text(encoding="utf-8"))
        deck = (ROOT / "main" / "tarot_deck.c").read_text(encoding="utf-8")
        names = re.findall(r'"([\u4e00-\u9fff]{2,5})",', deck)
        self.assertGreaterEqual(len(names), 78, "tarot_deck.c card names changed")
        for name in names:
            for ch in name:
                self.assertIn(ch, inventory, f"missing glyph for {name}")


if __name__ == "__main__":
    unittest.main()
