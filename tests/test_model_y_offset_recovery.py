import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GEOMETRY = ROOT / "decomp" / "include" / "comet" / "model_geometry.hpp"
PARAMS = ROOT / "decomp" / "src" / "model_load_parameters.cpp"
DOC = ROOT / "docs" / "decomp" / "model-y-offset.md"


class ModelYOffsetRecoveryTests(unittest.TestCase):
    def test_field_and_option_bit_are_pinned(self):
        text = GEOMETRY.read_text(encoding="utf-8")
        self.assertIn("vertex_y_offset = 0x28", text)
        self.assertIn("kLegacyModelOptionApplyVertexYOffset = 0x20", text)

    def test_native_helper_adjusts_only_y(self):
        text = PARAMS.read_text(encoding="utf-8")
        self.assertIn("original_options & kLegacyModelOptionApplyVertexYOffset", text)
        self.assertIn("position.y += vertex_y_offset", text)
        self.assertNotIn("position.x += vertex_y_offset", text)
        self.assertNotIn("position.z += vertex_y_offset", text)

    def test_exact_ppu_evidence_is_documented(self):
        text = DOC.read_text(encoding="utf-8")
        for addr in (
            "0x001074EC", "0x001075A4", "0x001075EC", "0x0010762C",
            "0x00107624", "0x0010AB40", "0x0010AB54",
        ):
            self.assertIn(addr, text)

    def test_original_option_name_is_not_invented(self):
        text = DOC.read_text(encoding="utf-8")
        self.assertIn("legacy numeric option", text)


if __name__ == "__main__":
    unittest.main()
