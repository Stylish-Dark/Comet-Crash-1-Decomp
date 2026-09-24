import sys, tempfile, unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import patch_ps3recomp_thread_exit as p

class ThreadExitPatchTests(unittest.TestCase):
    def test_windows_uses_endthreadex_and_posix_keeps_longjmp(self):
        src='before\n        '+p.OLD+'\nafter\n'
        out,changed=p.patch_text(src)
        self.assertTrue(changed)
        self.assertIn('_endthreadex(0)',out)
        self.assertIn('#else\n            longjmp(s_exit_jmp, 1);',out)
        out2,changed2=p.patch_text(out)
        self.assertFalse(changed2)
        self.assertEqual(out2,out)

    def test_changed_upstream_fails_loudly(self):
        with self.assertRaisesRegex(ValueError,'upstream changed'):
            p.patch_text('no unwind site')

    def test_patch_file_targets_runtime_syscall(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)
            path=root/'runtime'/'syscalls'/'sys_ppu_thread.c'
            path.parent.mkdir(parents=True)
            path.write_text(p.OLD,encoding='utf-8')
            self.assertTrue(p.patch_file(root))
            self.assertIn(p.MARKER,path.read_text(encoding='utf-8'))

if __name__=='__main__':
    unittest.main()
