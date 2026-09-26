import sys, tempfile, unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import patch_ps3recomp_unresolved as p

class UnresolvedGuestTextPatchTests(unittest.TestCase):
    def fixture(self):
        return 'prefix\n' + p.ANCHOR + 'suffix\n'

    def test_aligned_game_text_unresolved_target_fails_fast(self):
        out,changed=p.patch_text(self.fixture())
        self.assertTrue(changed)
        self.assertIn(p.MARKER,out)
        self.assertIn('(addr & 3u) == 0u',out)
        self.assertIn('addr >= 0x00010000u && addr < 0x10000000u',out)
        self.assertIn('ppu_dump_guest_stack(ctx, "unresolved-text")',out)
        self.assertIn('exit(3);',out)

    def test_patch_is_idempotent(self):
        once,changed=p.patch_text(self.fixture())
        self.assertTrue(changed)
        twice,changed2=p.patch_text(once)
        self.assertFalse(changed2)
        self.assertEqual(twice,once)

    def test_upstream_drift_fails_loudly(self):
        with self.assertRaisesRegex(ValueError,'upstream changed'):
            p.patch_text('no indirect-dispatch tail here')

    def test_patch_file_targets_ppu_loader(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)
            path=root/'runtime'/'ppu'/'ppu_loader.cpp'
            path.parent.mkdir(parents=True)
            path.write_text(self.fixture(),encoding='utf-8')
            self.assertTrue(p.patch_file(root))
            self.assertIn(p.MARKER,path.read_text(encoding='utf-8'))

if __name__=='__main__':
    unittest.main()
