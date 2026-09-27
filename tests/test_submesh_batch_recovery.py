import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "decomp" / "include" / "comet" / "submesh_batch.hpp"
DOC = ROOT / "docs" / "decomp" / "submesh-expanded-batch.md"


class ExpandedSubmeshBatchRecoveryTests(unittest.TestCase):
    def test_exact_legacy_tail_offsets_are_pinned(self):
        text = HEADER.read_text(encoding="utf-8")
        for token in (
            "enabled = 0x64",
            "instance_count = 0x68",
            "packed_vertex_stream = 0x6C",
            "index_stream_u32 = 0x70",
            "object_info_stream = 0x74",
        ):
            self.assertIn(token, text)

    def test_exact_stream_strides_are_pinned(self):
        text = HEADER.read_text(encoding="utf-8")
        self.assertIn("kExpandedBatchVertexStride = 0x20", text)
        self.assertIn("kExpandedBatchObjectInfoStride = 0x10", text)
        self.assertIn("kExpandedBatchIndexElementSize = 0x04", text)

    def test_original_shader_parameter_names_are_preserved(self):
        text = HEADER.read_text(encoding="utf-8")
        for name in ("position_tx", "normal_ty", "objInfo"):
            self.assertIn(f'"{name}"', text)

    def test_batch_size_equations_are_explicit(self):
        text = HEADER.read_text(encoding="utf-8")
        self.assertIn("expanded_vertex_count()", text)
        self.assertIn("expanded_index_count()", text)
        self.assertIn("packed_vertex_bytes()", text)
        self.assertIn("index_bytes()", text)
        self.assertIn("object_info_bytes()", text)

    def test_writer_reader_and_clear_anchors_are_documented(self):
        text = DOC.read_text(encoding="utf-8")
        for addr in (
            "0x000FF060",
            "0x000FF12C",
            "0x000FF134",
            "0x000FF138",
            "0x000FF13C",
            "0x001007E8",
            "0x001009C8",
            "0x000FF64C",
        ):
            self.assertIn(addr, text)

    def test_small_spu_link_is_not_overclaimed(self):
        text = DOC.read_text(encoding="utf-8")
        self.assertIn("not yet proven", text)

    def test_public_header_has_no_ps3_runtime_surface(self):
        text = HEADER.read_text(encoding="utf-8")
        for token in ("ppu_context", "cellSpurs", "psgl", "vm_write"):
            self.assertNotIn(token, text)


if __name__ == "__main__":
    unittest.main()
