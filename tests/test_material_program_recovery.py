import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "decomp" / "include" / "comet" / "material_programs.hpp"
SOURCE = ROOT / "decomp" / "src" / "material_programs.cpp"
DOC = ROOT / "docs" / "decomp" / "model-object.md"


class MaterialProgramRecoveryTests(unittest.TestCase):
    def test_exact_program_offsets_are_pinned(self):
        text = HEADER.read_text(encoding="utf-8")
        for token in (
            "standard_vertex_program = 0x48",
            "batched_vertex_program = 0x4C",
            "fragment_program = 0x50",
            "batch_enabled = 0x54",
        ):
            self.assertIn(token, text)

    def test_exact_filename_suffixes_are_pinned(self):
        text = HEADER.read_text(encoding="utf-8")
        for suffix in ('".vpo"', '"_spu.vpo"', '".fpo"'):
            self.assertIn(suffix, text)

    def test_native_builder_constructs_all_three_paths(self):
        text = SOURCE.read_text(encoding="utf-8")
        for token in (
            "kLegacyVertexProgramSuffix",
            "kLegacyBatchedVertexProgramSuffix",
            "kLegacyFragmentProgramSuffix",
        ):
            self.assertIn(token, text)

    def test_renderer_selection_is_documented(self):
        text = DOC.read_text(encoding="utf-8")
        for token in (
            "0x00101960",
            "material `+0x48`",
            "material `+0x4C`",
            "material `+0x50`",
            "standard `.vpo`",
            "`_spu.vpo`",
            "`.fpo`",
        ):
            self.assertIn(token, text)

    def test_header_has_no_ps3_runtime_surface(self):
        text = HEADER.read_text(encoding="utf-8")
        for token in ("ppu_context", "cellGcm", "psgl", "vm_write"):
            self.assertNotIn(token, text)


if __name__ == "__main__":
    unittest.main()
