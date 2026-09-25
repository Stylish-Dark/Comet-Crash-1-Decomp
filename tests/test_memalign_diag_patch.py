import sys, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import patch_comet_memalign_diag as p

class MemalignDiagPatchTests(unittest.TestCase):
    def fixture(self):
        return "\n".join([
            p.WRAP_ANCHOR,
            p.WRAP_RETURN_ANCHOR,
            p.CORE_ANCHOR,
            p.MALLOC_ANCHOR,
            p.FINAL_ANCHOR,
        ])+"\n"

    def test_instruments_wrapper_backing_malloc_and_core_return(self):
        out,changed=p.patch_text(self.fixture())
        self.assertTrue(changed)
        self.assertIn("[COMET-MEMALIGN-WRAPPER-LOW]",out)
        self.assertIn("[COMET-MEMALIGN-MALLOC-LOW]",out)
        self.assertIn("[COMET-MEMALIGN-CORE-LOW]",out)
        self.assertIn("malloc_request=0x%08X",out)
        out2,changed2=p.patch_text(out)
        self.assertFalse(changed2)
        self.assertEqual(out2,out)

    def test_lift_drift_fails_loudly(self):
        with self.assertRaises(ValueError):
            p.patch_text("no memalign anchors here")

if __name__=="__main__":
    unittest.main()
