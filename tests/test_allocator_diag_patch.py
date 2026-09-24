import sys, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import patch_comet_allocator_diag as p

class AllocatorDiagPatchTests(unittest.TestCase):
    def test_instruments_exact_free_abort_site_without_suppressing_abort(self):
        src=p.FUNC_ANCHOR+"\n...\n"+p.FAIL_ANCHOR+"\n"
        out,changed=p.patch_text(src)
        self.assertTrue(changed)
        self.assertIn(p.MARKER,out)
        self.assertIn("caller_lr=0x%08X",out)
        self.assertIn("func_0019427C(ctx)",out)
        out2,changed2=p.patch_text(out)
        self.assertFalse(changed2)
        self.assertEqual(out2,out)
    def test_upstream_or_lift_drift_fails_loudly(self):
        with self.assertRaises(ValueError):
            p.patch_text("no allocator anchors here")

if __name__=="__main__":
    unittest.main()
