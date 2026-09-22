import tempfile
import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import make_ci_scaffold as c


class CiScaffoldTests(unittest.TestCase):
    def test_stub_sources_are_non_proprietary_and_satisfy_cmake_layout(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            recomp = root / "recompiled"
            spu = root / "spu"
            registry = root / "spu_workloads.c"
            c.write_stub_sources(recomp, spu, registry)
            ppu = (recomp / "ppu_recomp_ci.cpp").read_text()
            self.assertIn("function_table", ppu)
            self.assertIn("function_table_count", ppu)
            self.assertIn("void func_00010000(ppu_context*)", ppu)
            self.assertNotIn('extern "C" void func_00010000', ppu)
            self.assertTrue((spu / "ci_stub" / "spu_recomp.c").is_file())
            self.assertTrue(registry.is_file())
            self.assertNotIn("NPEB00142", ppu)

    def test_stage_refuses_checkout_without_upstream_smoke_generator(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            with self.assertRaises(FileNotFoundError):
                c.stage(root / "not-ps3recomp", root / "generated", root / "work")


if __name__ == "__main__":
    unittest.main()
