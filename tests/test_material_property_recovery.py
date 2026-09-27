import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "decomp" / "include" / "comet" / "material_properties.hpp"
SOURCE = ROOT / "decomp" / "src" / "material_properties.cpp"
DOC = ROOT / "docs" / "decomp" / "material-properties.md"


class MaterialPropertyRecoveryTests(unittest.TestCase):
    def test_standard_mtl_directives_are_mapped(self):
        text = SOURCE.read_text(encoding="utf-8")
        for keyword in ("Ka", "Kd", "Ks", "Ns"):
            self.assertIn(f'"{keyword}"', text)

    def test_exact_property_offsets_are_preserved(self):
        text = HEADER.read_text(encoding="utf-8")
        expected = (
            "ambient_r = 0x00",
            "ambient_g = 0x04",
            "ambient_b = 0x08",
            "ambient_w = 0x0C",
            "diffuse_r = 0x10",
            "diffuse_g = 0x14",
            "diffuse_b = 0x18",
            "diffuse_w = 0x1C",
            "specular_r = 0x20",
            "specular_g = 0x24",
            "specular_b = 0x28",
            "specular_w = 0x2C",
            "scaled_specular_exponent = 0x30",
            "use_team_color = 0x34",
        )
        for token in expected:
            self.assertIn(token, text)

    def test_exact_ns_scale_is_pinned(self):
        text = HEADER.read_text(encoding="utf-8")
        self.assertIn("0.12800000607967377f", text)
        doc = DOC.read_text(encoding="utf-8")
        self.assertIn("0x0010A658", doc)
        self.assertIn("0x0010A670", doc)

    def test_vec4_w_lanes_are_proven_zero_initialized(self):
        text = DOC.read_text(encoding="utf-8")
        self.assertIn("0x00107150..0x001071B4", text)
        for name in ("colorDiffuse", "colorAmbient", "colorSpecular"):
            self.assertIn(name, text)
        self.assertIn("named **W**, not alpha", text)

    def test_team_color_field_and_prefix_policy_are_recovered(self):
        header = HEADER.read_text(encoding="utf-8")
        doc = DOC.read_text(encoding="utf-8")
        self.assertIn("material_name_uses_team_color", header)
        self.assertIn('"useTeamColor"', doc)
        self.assertIn("0x001097B8", doc)
        self.assertIn("0x0010A944", doc)
        self.assertIn("newmtl team.light", doc)

    def test_public_header_has_no_ps3_runtime_api(self):
        text = HEADER.read_text(encoding="utf-8")
        for token in ("ppu_context", "psgl", "cellGcm", "vm_write"):
            self.assertNotIn(token, text)


if __name__ == "__main__":
    unittest.main()
