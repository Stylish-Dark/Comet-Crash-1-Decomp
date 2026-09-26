import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "decomp" / "include" / "comet" / "model_geometry.hpp"
SOURCE = ROOT / "decomp" / "src" / "model_geometry.cpp"
DOC = ROOT / "docs" / "decomp" / "model-object.md"


class ModelGeometryRecoveryTests(unittest.TestCase):
    def test_proven_top_level_offsets_are_locked(self):
        text = HEADER.read_text(encoding="utf-8")
        expected = (
            "vertex_count = 0x04",
            "submesh_count = 0x08",
            "index_count = 0x0C",
            "full_vertices = 0x10",
            "compact_vertices = 0x14",
            "submeshes = 0x18",
            "indices = 0x1C",
            "full_vertex_buffer = 0x20",
            "compact_vertex_buffer = 0x24",
        )
        for token in expected:
            self.assertIn(token, text)

    def test_exact_legacy_strides_are_preserved(self):
        text = HEADER.read_text(encoding="utf-8")
        for token in ("0x20", "0x14", "0x78", "0x0C", "0x02"):
            self.assertIn(token, text)

    def test_full_and_compact_vertex_formats(self):
        text = SOURCE.read_text(encoding="utf-8")
        self.assertIn("VertexSemantic::Position, 0x00, 3", text)
        self.assertIn("VertexSemantic::Normal, 0x0C, 3", text)
        self.assertIn("VertexSemantic::TexCoord0, 0x18, 2", text)
        self.assertIn("VertexSemantic::TexCoord0, 0x0C, 2", text)

    def test_material_subobject_correction_is_documented(self):
        text = DOC.read_text(encoding="utf-8")
        self.assertIn("does **not** receive the root model object", text)
        self.assertIn("submesh + 0x10", text)
        self.assertIn("material_vector_end - 0x68", text)

    def test_native_header_has_no_ps3_runtime_surface(self):
        text = HEADER.read_text(encoding="utf-8")
        for token in ("ppu_context", "cellGcm", "psgl", "vm_read", "vm_write"):
            self.assertNotIn(token, text)


if __name__ == "__main__":
    unittest.main()
