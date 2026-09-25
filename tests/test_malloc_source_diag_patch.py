import sys, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import patch_comet_malloc_source_diag as p

class MallocSourceDiagPatchTests(unittest.TestCase):
    def fixture(self):
        lines=[p.FUNC_ANCHOR]
        for i in range(p.EXPECTED_TRACKED_R31_WRITES):
            lines += [f"loc_{0x001A6000+i*4:08X}:", f"        ctx->gpr[31] = ctx->gpr[{i%31}] + (int64_t)(8);"]
        lines += [p.RETURN_ANCHOR, "        ctx->gpr[31] = _cs_31;", "        return;", "}", "void func_001A8000(ppu_context* ctx) {}"]
        return "\n".join(lines)+"\n"

    def test_tags_every_runtime_r31_producer_and_logs_low_return(self):
        out,changed=p.patch_text(self.fixture())
        self.assertTrue(changed)
        self.assertIn("[COMET-MALLOC-LOW]",out)
        self.assertIn("[COMET-MALLOC-STATE]",out)
        self.assertIn("source=%s",out)
        self.assertIn("request=0x%08X",out)
        self.assertEqual(out.count('comet_malloc_r31_source="loc_'),p.EXPECTED_TRACKED_R31_WRITES)
        self.assertIn("chunk=result >= 8u ? result-8u : 0u;",out)
        out2,changed2=p.patch_text(out)
        self.assertFalse(changed2)
        self.assertEqual(out2,out)

    def test_r31_producer_count_drift_fails_loudly(self):
        bad=self.fixture().replace("        ctx->gpr[31] = ctx->gpr[0] + (int64_t)(8);\n","",1)
        with self.assertRaisesRegex(ValueError,"tracked Comet mspace_malloc r31 writes"):
            p.patch_text(bad)

    def test_missing_function_anchor_fails_loudly(self):
        with self.assertRaises(ValueError):
            p.patch_text("no malloc anchors here")

if __name__=="__main__":
    unittest.main()
