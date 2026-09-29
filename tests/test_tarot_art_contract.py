#!/usr/bin/env python3
"""Embedded artwork contract for the tarot application.

Validates the committed assets/images/tarot_art*.{c,h} against the deck:
78 arrays at each resolution (full 120x180, mini 64x96) with exact pixel
counts, one descriptor per card id, and the generator's id-to-file mapping
staying aligned with the deck module. Pure stdlib (no Pillow) so the gate
can run anywhere.
"""

from pathlib import Path
import importlib.util
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
ART_H = ROOT / "assets" / "images" / "tarot_art.h"
ART_FULL = ROOT / "assets" / "images" / "tarot_art_img.c"
ART_MINI = ROOT / "assets" / "images" / "tarot_art_mini.c"
GEN = ROOT / "tools" / "gen_tarot_art.py"

SPEC = importlib.util.spec_from_file_location("gen_tarot_art", GEN)
assert SPEC and SPEC.loader
GEN_MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GEN_MODULE)


class TarotArtContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.header = ART_H.read_text(encoding="utf-8")
        cls.full = ART_FULL.read_text(encoding="utf-8")
        cls.mini = ART_MINI.read_text(encoding="utf-8")

    def test_mapping_covers_78_cards_in_deck_order(self) -> None:
        pairs = GEN_MODULE.card_sources()
        self.assertEqual([card_id for card_id, _ in pairs], list(range(78)))
        majors = [name for _, name in pairs[:22]]
        self.assertEqual(majors, ["maj%02d.jpg" % i for i in range(22)])
        for offset, stem in ((22, "wands"), (36, "cups"),
                             (50, "swords"), (64, "pents")):
            self.assertEqual(
                [name for _, name in pairs[offset:offset + 14]],
                ["%s%02d.jpg" % (stem, i) for i in range(1, 15)])

    def check_set(self, text, prefix, width, height, descriptor) -> None:
        arrays = re.findall(
            r"static const uint16_t %s_(\d+)\[(\d+)\]" % prefix, text)
        self.assertEqual(len(arrays), 78)
        seen = set()
        for card_id, count in arrays:
            self.assertEqual(int(count), width * height)
            seen.add(int(card_id))
        self.assertEqual(seen, set(range(78)))

        entries = re.findall(r"(?m)^    \[(\d+)\] = \{$", text)
        self.assertEqual(sorted(int(i) for i in entries), list(range(78)))
        self.assertIn(".header.w = %d" % width, text)
        self.assertIn(".header.h = %d" % height, text)
        self.assertIn("LV_COLOR_FORMAT_RGB565", text)
        self.assertIn("LV_IMAGE_HEADER_MAGIC", text)
        self.assertIn("const lv_image_dsc_t %s[TAROT_ART_COUNT]" % descriptor,
                      text)

    def test_full_set_78_cards_120x180(self) -> None:
        self.check_set(self.full, "k_tarot_card", GEN_MODULE.FULL_W,
                       GEN_MODULE.FULL_H, "tarot_card_images")

    def test_mini_set_78_cards_64x96(self) -> None:
        self.check_set(self.mini, "k_tarot_card_mini", GEN_MODULE.MINI_W,
                       GEN_MODULE.MINI_H, "tarot_card_mini_images")

    def test_header_exports_both_descriptor_arrays(self) -> None:
        self.assertIn("#define TAROT_ART_COUNT 78", self.header)
        self.assertIn("#define TAROT_ART_W %d" % GEN_MODULE.FULL_W, self.header)
        self.assertIn("#define TAROT_ART_H %d" % GEN_MODULE.FULL_H, self.header)
        self.assertIn("#define TAROT_ART_MINI_W %d" % GEN_MODULE.MINI_W,
                      self.header)
        self.assertIn("#define TAROT_ART_MINI_H %d" % GEN_MODULE.MINI_H,
                      self.header)
        self.assertIn("tarot_card_images[TAROT_ART_COUNT]", self.header)
        self.assertIn("tarot_card_mini_images[TAROT_ART_COUNT]", self.header)


if __name__ == "__main__":
    unittest.main()
