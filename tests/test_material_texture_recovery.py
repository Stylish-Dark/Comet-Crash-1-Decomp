import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "decomp" / "include" / "comet" / "material_textures.hpp"
SOURCE = ROOT / "decomp" / "src" / "material_textures.cpp"
DOC = ROOT / "docs" / "decomp" / "material-textures.md"


class MaterialTextureRecoveryTests(unittest.TestCase):
    def test_all_four_mtl_directives_are_native(self):
        text = SOURCE.read_text(encoding="utf-8")
        for keyword in ("map_Kd", "map_Ks", "bump", "cube"):
            self.assertIn(f'"{keyword}"', text)

    def test_exact_material_offsets_are_preserved(self):
        text = HEADER.read_text(encoding="utf-8")
        for token in (
            "diffuse = 0x38",
            "specular = 0x3C",
            "bump = 0x40",
            "environment_cube = 0x44",
            "shader = 0x48",
        ):
            self.assertIn(token, text)

    def test_cube_uses_distinct_loader_kind(self):
        text = SOURCE.read_text(encoding="utf-8")
        self.assertIn(
            'MaterialTextureSemantic::EnvironmentCube,\n'
            '               MaterialTextureLoaderKind::CubeTexture',
            text,
        )

    def test_store_anchors_are_documented(self):
        text = DOC.read_text(encoding="utf-8")
        for anchor in ("0x0010A8F8", "0x0010AED8", "0x0010B174", "0x0010B368"):
            self.assertIn(anchor, text)

    def test_public_header_has_no_ps3_runtime_api(self):
        text = HEADER.read_text(encoding="utf-8")
        for token in ("ppu_context", "psgl", "cellGcm", "vm_write"):
            self.assertNotIn(token, text)


if __name__ == "__main__":
    unittest.main()
