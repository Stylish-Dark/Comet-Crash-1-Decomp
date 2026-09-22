import json
import tempfile
import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import make_ci_scaffold as c


class CiScaffoldTests(unittest.TestCase):
    def test_synthetic_ppu_input_is_minimal_big_endian_blr(self):
        with tempfile.TemporaryDirectory() as td:
            raw, functions = c.write_ppu_input(Path(td))
            self.assertEqual(raw.read_bytes(), bytes.fromhex("4e800020"))
            self.assertEqual(
                json.loads(functions.read_text()),
                [{"start": "0x10000", "end": "0x10004"}],
            )

    def test_spu_stub_sources_are_non_proprietary_and_satisfy_cmake_layout(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            spu = root / "spu"
            registry = root / "spu_workloads.c"
            c.write_spu_stub_sources(spu, registry)
            self.assertTrue((spu / "ci_stub" / "spu_recomp.c").is_file())
            self.assertTrue(registry.is_file())
            self.assertNotIn("NPEB00142", registry.read_text())

    def test_stage_refuses_checkout_without_upstream_lifter(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            with self.assertRaises(FileNotFoundError):
                c.stage(root / "not-ps3recomp", root / "generated", root / "work")


if __name__ == "__main__":
    unittest.main()
