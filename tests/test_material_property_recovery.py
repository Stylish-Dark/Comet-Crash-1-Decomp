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
            "diffuse_r = 0x10",
            "diffuse_g = 0x14",
            "diffuse_b = 0x18",
            "specular_r = 0x20",
            "specular_g = 0x24",
            "specular_b = 0x28",
            "scaled_specular_exponent = 0x30",
        )
        for token in expected:
            self.assertIn(token, text)

    def test_exact_ns_scale_is_pinned(self):
        text = HEADER.read_text(encoding="utf-8")
        self.assertIn("0.12800000607967377f", text)
        doc = DOC.read_text(encoding="utf-8")
        self.assertIn("0x0010A658", doc)
        self.assertIn("0x0010A670", doc)

    def test_unknown_gap_fields_are_not_promoted(self):
        text = DOC.read_text(encoding="utf-8")
        self.assertIn("+0x0C/+0x1C/+0x2C/+0x34", text)
        self.assertIn("left unnamed", text)

    def test_public_header_has_no_ps3_runtime_api(self):
        text = HEADER.read_text(encoding="utf-8")
        for token in ("ppu_context", "psgl", "cellGcm", "vm_write"):
            self.assertNotIn(token, text)


if __name__ == "__main__":
    unittest.main()
