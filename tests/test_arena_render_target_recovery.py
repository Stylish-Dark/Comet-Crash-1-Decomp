import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "decomp" / "include" / "comet" / "arena_render_targets.hpp"
SOURCE = ROOT / "decomp" / "src" / "arena_render_targets.cpp"
DOC = ROOT / "docs" / "decomp" / "arena-render-targets.md"


class ArenaRenderTargetRecoveryTests(unittest.TestCase):
    def test_source_line_anchors_are_preserved(self):
        text = DOC.read_text(encoding="utf-8")
        for line in (1034, 1081, 1100, 1115, 1130, 1143, 1188):
            self.assertIn(f"arenaGraphics.cpp:{line}", text)

    def test_native_plan_keeps_proven_resource_offsets(self):
        text = SOURCE.read_text(encoding="utf-8")
        for offset in (
            "0x4490", "0x4498", "0x448C", "0x449C", "0x44B8",
            "0x44A0", "0x44A4", "0x44A8", "0x44AC", "0x44B0",
            "0x44B4",
        ):
            self.assertIn(offset, text)
        for offset in (
            "0x445C", "0x4468", "0x446C", "0x4470", "0x4474",
            "0x4478", "0x447C", "0x4480", "0x4484", "0x4488",
        ):
            self.assertIn(offset, text)

    def test_mode_scale_policy_is_explicit(self):
        text = SOURCE.read_text(encoding="utf-8")
        for legacy_value in ("0x6030", "0x6031", "0x6032", "0x6033"):
            self.assertIn(legacy_value, text)
        self.assertIn("display_scale_x = 2", text)
        self.assertIn("display_scale_y = 1", text)

    def test_renderer_api_does_not_expose_psgl_calls(self):
        header = HEADER.read_text(encoding="utf-8")
        for token in (
            "glBindTexture",
            "glTexImage2D",
            "glBindFramebuffer",
            "cellGcm",
            "psgl",
        ):
            self.assertNotIn(token, header)


if __name__ == "__main__":
    unittest.main()
