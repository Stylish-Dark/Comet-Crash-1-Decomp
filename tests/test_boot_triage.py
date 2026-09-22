import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import boot_triage as b


class BootTriageTests(unittest.TestCase):
    def test_hle_signal_classifies_hle(self):
        category, why = b.classify_signal("[HLE] UNIMPLEMENTED nid=0x12345678")
        self.assertEqual(category, "hle")
        self.assertIn("unimplemented", why.lower())

    def test_rsx_signal_classifies_graphics(self):
        category, _ = b.classify_signal("D3D12 init FAILED hr=0x80004005")
        self.assertEqual(category, "gcm/resc/rsx")

    def test_generic_watchdog_does_not_guess_subsystem(self):
        category, why = b.classify_signal("[watchdog] no guest frame after 10s")
        self.assertEqual(category, "unknown")
        self.assertIn("does not identify", why)

    def test_footer_overrides_stream_summary(self):
        lines = [
            "[boot-stage] process entered",
            "[boot-stage] entering recompiled title",
            "[watchdog] no guest frame after 10s",
            "# last_boot_stage=HLE NID table initialized",
            "# first_signal=[HLE] UNIMPLEMENTED nid=0xDEADBEEF",
            "# host_exit_code=7",
        ]
        report = b.summarize_lines(lines)
        self.assertEqual(report["last_boot_stage"], "HLE NID table initialized")
        self.assertEqual(report["first_signal"], "[HLE] UNIMPLEMENTED nid=0xDEADBEEF")
        self.assertEqual(report["triage_signal"], "[HLE] UNIMPLEMENTED nid=0xDEADBEEF")
        self.assertEqual(report["host_exit_code"], 7)
        self.assertEqual(report["suspected_subsystem"], "hle")

    def test_later_specific_signal_overrides_generic_first_symptom(self):
        report = b.summarize_lines([
            "[boot-stage] entering recompiled title",
            "[watchdog] no guest frame after 10s",
            "[HLE] UNIMPLEMENTED nid=0xCAFEBABE",
            "[crash] code=0xC0000005",
            "# host_exit_code=9",
        ])
        self.assertEqual(report["first_signal"], "[watchdog] no guest frame after 10s")
        self.assertEqual(report["triage_signal"], "[HLE] UNIMPLEMENTED nid=0xCAFEBABE")
        self.assertEqual(report["suspected_subsystem"], "hle")

    def test_unsupported_spu_is_specific(self):
        category, _ = b.classify_signal("unsupported SPU opcode at 0x100")
        self.assertEqual(category, "spurs/spu")

    def test_first_frame_is_reported(self):
        report = b.summarize_lines([
            "[boot-stage] entering recompiled title",
            "[boot-stage] first guest frame presented",
            "# host_exit_code=0",
        ])
        self.assertTrue(report["first_frame_presented"])
        self.assertEqual(report["host_exit_code"], 0)
        self.assertEqual(report["suspected_subsystem"], "unknown")

    def test_summarize_file_reads_saved_boot_log(self):
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "boot.txt"
            p.write_text(
                "[boot-stage] entering recompiled title\n"
                "HOTREAD spin at guest=0x1000\n"
                "# host_exit_code=9\n",
                encoding="utf-8",
            )
            report = b.summarize_file(p)
            self.assertEqual(report["suspected_subsystem"], "synchronization")
            self.assertEqual(report["host_exit_code"], 9)


if __name__ == "__main__":
    unittest.main()
