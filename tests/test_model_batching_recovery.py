import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "decomp" / "include" / "comet" / "model_batching.hpp"
DOC = ROOT / "docs" / "decomp" / "model-batching.md"


class ModelBatchingRecoveryTests(unittest.TestCase):
    def test_legacy_batch_offsets_are_pinned(self):
        text = HEADER.read_text(encoding="utf-8")
        for token in (
            "batch_enabled = 0x64",
            "batched_instance_count = 0x68",
            "batched_vertex_stream = 0x6C",
            "batched_index_stream = 0x70",
            "batched_object_info_stream = 0x74",
        ):
            self.assertIn(token, text)

    def test_exact_generated_stream_sizes_are_pinned(self):
        text = HEADER.read_text(encoding="utf-8")
        self.assertIn("kLegacyBatchedVertexStride = 0x20", text)
        self.assertIn("kLegacyBatchedIndexElementSize = 0x04", text)
        self.assertIn("kLegacyBatchedObjectInfoStride = 0x10", text)
        self.assertIn("instance_count) *", text)

    def test_exact_shader_parameters_are_documented(self):
        text = DOC.read_text(encoding="utf-8")
        for token in (
            "normal_ty",
            "objInfo",
            "POSITION",
            "TEXCOORD0",
            "TEXCOORD1",
            "lit_texture_shader_spu.vpo",
        ):
            self.assertIn(token, text)

    def test_renderer_and_prep_anchors_are_recorded(self):
        text = DOC.read_text(encoding="utf-8")
        for anchor in (
            "0x000FEFB0",
            "0x000FF060",
            "0x000FF068",
            "0x000FF12C",
            "0x00100750",
            "0x001009E8",
        ):
            self.assertIn(anchor, text)

    def test_native_header_does_not_expose_spu_runtime(self):
        text = HEADER.read_text(encoding="utf-8")
        for token in ("cellSpurs", "ppu_context", "spu_thread", "vm_write"):
            self.assertNotIn(token, text)


if __name__ == "__main__":
    unittest.main()
