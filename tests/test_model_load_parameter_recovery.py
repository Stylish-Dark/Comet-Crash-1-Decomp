import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ASSET_HEADER = ROOT / "decomp" / "include" / "comet" / "arena_assets.hpp"
PARAM_HEADER = ROOT / "decomp" / "include" / "comet" / "model_load_parameters.hpp"
PARAM_SOURCE = ROOT / "decomp" / "src" / "model_load_parameters.cpp"
DOC = ROOT / "docs" / "decomp" / "model-load-parameters.md"


class ModelLoadParameterRecoveryTests(unittest.TestCase):
    def test_arena_manifest_uses_semantic_parameter_names(self):
        text = ASSET_HEADER.read_text(encoding="utf-8")
        self.assertIn("geometry_scale", text)
        self.assertIn("signed_radius_scale", text)
        self.assertNotIn("original_param1", text)
        self.assertNotIn("original_param2", text)

    def test_geometry_scale_is_explicit(self):
        text = PARAM_SOURCE.read_text(encoding="utf-8")
        self.assertIn("position.x *= geometry_scale", text)
        self.assertIn("position.y *= geometry_scale", text)
        self.assertIn("position.z *= geometry_scale", text)

    def test_signed_radius_algorithm_is_explicit(self):
        text = PARAM_SOURCE.read_text(encoding="utf-8")
        self.assertIn("std::fabs(signed_radius_scale)", text)
        self.assertIn("std::sqrt", text)
        self.assertIn("candidate > radius", text)
        self.assertIn("signed_radius_scale < 0.0f ? -radius : radius", text)

    def test_exact_ppu_evidence_is_documented(self):
        text = DOC.read_text(encoding="utf-8")
        for addr in (
            "0x001053A4", "0x0010751C", "0x00107580",
            "0x001053B4", "0x00105448", "0x001076C8", "0x00105D54",
        ):
            self.assertIn(addr, text)

    def test_public_header_has_no_ps3_runtime_api(self):
        text = PARAM_HEADER.read_text(encoding="utf-8")
        for token in ("ppu_context", "psgl", "cellGcm", "vm_write"):
            self.assertNotIn(token, text)


if __name__ == "__main__":
    unittest.main()
