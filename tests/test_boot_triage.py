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

    def test_specific_d3d12_error_is_captured_before_generic_init_failure(self):
        report=b.summarize_lines([
            "[D3D12] ERROR: CreateCommandQueue failed (0x8007000E)",
            "[rsx] D3D12 init FAILED (1280x720)",
            "# host_exit_code=9",
        ])
        self.assertEqual(report["first_signal"],"[D3D12] ERROR: CreateCommandQueue failed (0x8007000E)")
        self.assertEqual(report["triage_signal"],"[D3D12] ERROR: CreateCommandQueue failed (0x8007000E)")
        self.assertEqual(report["suspected_subsystem"],"gcm/resc/rsx")

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
        self.assertEqual(report["boot_outcome"], "failure-before-frame")

    def test_footer_lines_are_not_reparsed_as_runtime_signals(self):
        report = b.summarize_lines([
            "[watchdog] no guest frame after 10s",
            "# first_signal=[HLE] UNIMPLEMENTED nid=0xDEADBEEF",
            "# host_exit_code=7",
        ])
        self.assertEqual(report["first_signal"], "[HLE] UNIMPLEMENTED nid=0xDEADBEEF")
        self.assertEqual(report["triage_signal"], "[HLE] UNIMPLEMENTED nid=0xDEADBEEF")
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

    def test_memalign_origin_prefers_earliest_producing_stage(self):
        report = b.summarize_lines([
            "[COMET-MEMALIGN-MALLOC-LOW] result=0x00000140 least=0x40000000",
            "[COMET-MEMALIGN-CORE-LOW] result=0x00000140 least=0x40000000",
            "[COMET-MEMALIGN-WRAPPER-LOW] result=0x00000140 least=0x40000000",
            "[COMET-ALLOC-CORRUPTION] mem=0x00000140 least=0x40000000",
            "# host_exit_code=9",
        ])
        self.assertEqual(report["memalign_origin"], "backing-malloc")
        self.assertIn("MEMALIGN-MALLOC-LOW", report["memalign_signal"])
        self.assertEqual(report["triage_signal"], report["memalign_signal"])
        self.assertEqual(report["suspected_subsystem"], "vm/ppu")

    def test_memalign_core_is_origin_when_backing_malloc_is_not_low(self):
        report = b.summarize_lines([
            "[COMET-MEMALIGN-CORE-LOW] result=0x00000140 least=0x40000000",
            "[COMET-MEMALIGN-WRAPPER-LOW] result=0x00000140 least=0x40000000",
            "# host_exit_code=9",
        ])
        self.assertEqual(report["memalign_origin"], "alignment-core")
        self.assertIn("MEMALIGN-CORE-LOW", report["memalign_signal"])

    def test_malloc_low_captures_exact_return_producer(self):
        report=b.summarize_lines([
            "[COMET-MALLOC-LOW] caller_lr=0x001A7708 source=loc_001A61A8#1 request=0x00000390 mspace=0x00722220 least=0x40000000 result=0x00000140",
            "# host_exit_code=9",
        ])
        self.assertEqual(report["malloc_source"],"loc_001A61A8#1")
        self.assertIn("COMET-MALLOC-LOW",report["malloc_signal"])
        self.assertEqual(report["triage_signal"],report["malloc_signal"])
        self.assertEqual(report["suspected_subsystem"],"vm/ppu")

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
        self.assertEqual(report["boot_outcome"], "clean-visible-exit")

    def test_timeout_and_interrupt_outcomes_override_exit_code(self):
        self.assertEqual(
            b.derive_outcome(first_frame_presented=False,host_exit_code=1,timed_out=True),
            "timed-out",
        )
        self.assertEqual(
            b.derive_outcome(first_frame_presented=False,host_exit_code=1,interrupted="keyboard"),
            "interrupted",
        )

    def test_clean_exit_before_frame_is_distinct(self):
        self.assertEqual(
            b.derive_outcome(first_frame_presented=False,host_exit_code=0),
            "clean-exit-before-frame",
        )

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


    def test_parse_sp_change_is_specific_and_extracts_site(self):
        report=b.summarize_lines([
            "[COMET-PARSE-SP-CHANGE] site=0x00130378 expected_sp=0xD00717E0 got_sp=0xD0071810 slot=0xD0071864 slot_now=0x43C81280",
            "# host_exit_code=9",
        ])
        self.assertEqual(report["parse_corruption_kind"],"sp-change")
        self.assertEqual(report["parse_corruption_site"],"0X00130378")
        self.assertEqual(report["suspected_subsystem"],"vm/ppu")

    def test_parse_slot_change_is_specific_and_extracts_site(self):
        report=b.summarize_lines([
            "[COMET-PARSE-SLOT-CHANGE] site=0x00130204 callee=0x0001C7D4 slot=0xD0071864 expected=0x43C81280 got=0x00000140 sp=0xD00717E0",
            "# host_exit_code=9",
        ])
        self.assertEqual(report["parse_corruption_kind"],"slot-change")
        self.assertEqual(report["parse_corruption_site"],"0X00130204")
        self.assertIn("SLOT-CHANGE",report["parse_corruption_signal"])


if __name__ == "__main__":
    unittest.main()
