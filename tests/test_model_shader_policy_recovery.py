import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "decomp" / "include" / "comet" / "model_shader_policy.hpp"
SOURCE = ROOT / "decomp" / "src" / "model_shader_policy.cpp"
DOC = ROOT / "docs" / "decomp" / "model-object.md"


class ModelShaderPolicyRecoveryTests(unittest.TestCase):
    def test_all_surviving_shader_names_are_preserved(self):
        text = SOURCE.read_text(encoding="utf-8")
        names = (
            "lit_object_shader",
            "lit_object_shader_no_team_ground",
            "lit_texture_shader",
            "lit_bump_shader",
            "lit_texture_spec_shader",
            "lit_texture_spec_gloss_shader",
            "lit_bump_spec_shader",
            "lit_bump_spec_shader_no_team",
            "lit_bump_spec_shader_no_team_ground",
            "lit_bump_spec_gloss_shader_no_team",
            "lit_bump_spec_gloss_glow_shader_no_team",
        )
        for name in names:
            self.assertIn(f'"{name}"', text)

    def test_exact_option_masks_and_precedence_are_visible(self):
        text = SOURCE.read_text(encoding="utf-8")
        for mask in ("0x0Cu", "0x244u", "0x44u", "0x4u", "0x40u"):
            self.assertIn(mask, text)
        self.assertLess(text.index("0x0Cu"), text.index("0x244u"))
        self.assertLess(text.index("0x244u"), text.index("0x44u"))

    def test_proven_material_offsets_are_documented(self):
        header = HEADER.read_text(encoding="utf-8")
        for offset in ("0x38", "0x3C", "0x40", "0x48"):
            self.assertIn(offset, header)

    def test_public_policy_is_native_not_ps3_runtime(self):
        header = HEADER.read_text(encoding="utf-8")
        for token in ("ppu_context", "cellGcm", "psgl", "r3", "r4"):
            self.assertNotIn(token, header)

    def test_full_helper_boundary_is_recorded(self):
        text = DOC.read_text(encoding="utf-8")
        self.assertIn("0x00105088", text)
        self.assertIn("0x00105307", text)


if __name__ == "__main__":
    unittest.main()
