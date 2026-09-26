import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "decomp" / "include" / "comet" / "arena_assets.hpp"
SOURCE = ROOT / "decomp" / "src" / "arena_assets.cpp"
DOC = ROOT / "docs" / "decomp" / "arena-assets.md"


class ArenaAssetRecoveryTests(unittest.TestCase):
    def test_manifest_has_all_37_model_calls(self):
        text = SOURCE.read_text(encoding="utf-8")
        self.assertIn("std::array<ArenaModelAssetSpec, 37>", text)
        self.assertEqual(text.count('{"models/'), 37)

    def test_regular_0x90_slot_provenance_is_locked(self):
        header = HEADER.read_text(encoding="utf-8")
        source = SOURCE.read_text(encoding="utf-8")
        self.assertIn("0x2D2DC0", header)
        self.assertIn("0x90", header)
        self.assertIn("provenance_offsets_match_slots", source)

    def test_key_assets_and_exact_options_are_preserved(self):
        text = SOURCE.read_text(encoding="utf-8")
        required = (
            '"models/care/playerShip.obj"',
            '"models/care/resourceGeode/resourceGeode.obj"',
            '"models/care/weapons/structTurretStand.obj"',
            '"models/care/platforms/platBasic.obj"',
            '"models/gateway.obj"',
        )
        for item in required:
            self.assertIn(item, text)
        for option in ("0x003", "0x063", "0x043", "0x245"):
            self.assertIn(option, text)

    def test_public_manifest_has_no_ps3_runtime_api(self):
        text = HEADER.read_text(encoding="utf-8")
        for token in ("cellGcm", "cellSpurs", "psgl", "ppu_context", "r25"):
            self.assertNotIn(token, text)

    def test_documented_bootstrap_strings(self):
        text = DOC.read_text(encoding="utf-8")
        for item in (
            "wmd_font_b.fnt",
            "font_outlined_shader",
            "shaders.bin",
            "particle_effects.dds",
            "gui_quad_shader",
        ):
            self.assertIn(item, text)


if __name__ == "__main__":
    unittest.main()
