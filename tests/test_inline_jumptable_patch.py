import sys, tempfile, unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))

class InlineJumpTableLifterPatchTests(unittest.TestCase):
    def test_patch_adds_stack_spilled_inline_relative_fallback(self):
        import patch_ps3recomp_inline_jumptable as p
        src = 'prefix\n' + p.ANCHOR + 'suffix\n'
        out, changed = p.patch_text(src)
        self.assertTrue(changed)
        self.assertIn(p.MARKER, out)
        self.assertIn('_base = (all_insns[i].addr + 4) & 0xFFFFFFFF', out)
        self.assertIn('_off32 = _v - (1 << 32)', out)
        self.assertIn('tables[all_insns[i].addr] = sorted(set(_targets))', out)
        self.assertIn('if len(_targets) >= 2:', out)
        self.assertNotEqual(out, src)

    def test_patch_is_idempotent(self):
        import patch_ps3recomp_inline_jumptable as p
        src = 'prefix\n' + p.ANCHOR + 'suffix\n'
        once, changed = p.patch_text(src)
        self.assertTrue(changed)
        twice, changed2 = p.patch_text(once)
        self.assertFalse(changed2)
        self.assertEqual(twice, once)

    def test_patch_rejects_drifted_pin(self):
        import patch_ps3recomp_inline_jumptable as p
        with self.assertRaises(ValueError):
            p.patch_text('def discover_jump_tables():\n    pass\n')

    def test_file_round_trip(self):
        import patch_ps3recomp_inline_jumptable as p
        with tempfile.TemporaryDirectory() as td:
            f=Path(td)/'ppu_lifter.py'
            f.write_text(p.ANCHOR, encoding='utf-8')
            self.assertTrue(p.patch_file(f))
            self.assertFalse(p.patch_file(f))
            self.assertIn(p.MARKER, f.read_text(encoding='utf-8'))

if __name__ == '__main__':
    unittest.main()
