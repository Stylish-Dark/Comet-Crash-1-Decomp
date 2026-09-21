import contextlib
import io
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import comet_port


class BootLogTests(unittest.TestCase):
    def test_run_logged_mirrors_combined_output_and_metadata(self):
        with tempfile.TemporaryDirectory() as td:
            log=Path(td)/'boot.txt'
            out=io.StringIO()
            code='import sys; print("stdout-line"); print("stderr-line", file=sys.stderr)'
            with contextlib.redirect_stdout(out):
                rc=comet_port.run_logged(
                    [sys.executable,'-c',code],
                    log,
                    metadata={'marker':'unit-test'},
                )
            self.assertEqual(rc,0)
            text=log.read_text(encoding='utf-8')
            self.assertIn('stdout-line',text)
            self.assertIn('stderr-line',text)
            self.assertIn('"marker": "unit-test"',text)
            self.assertIn('# host_exit_code=0',text)
            self.assertIn('stdout-line',out.getvalue())

    def test_run_logged_records_nonzero_exit(self):
        with tempfile.TemporaryDirectory() as td:
            log=Path(td)/'boot.txt'
            with self.assertRaises(subprocess.CalledProcessError):
                comet_port.run_logged([sys.executable,'-c','raise SystemExit(7)'],log)
            self.assertIn('# host_exit_code=7',log.read_text(encoding='utf-8'))

    def test_run_parser_accepts_explicit_log_path(self):
        args=comet_port.parser().parse_args(['run','game','EBOOT.ELF','--log','logs/custom.txt'])
        self.assertEqual(args.log,Path('logs/custom.txt'))

    def test_native_runner_has_stage_markers_and_first_frame_marker(self):
        src=(ROOT/'port'/'main.cpp').read_text(encoding='utf-8')
        for marker in [
            'process entered',
            'guest VM reserved',
            'PPU ELF loaded',
            'HLE NID table initialized',
            'Comet HLE overrides installed',
            'entering recompiled title',
            'first guest frame presented',
        ]:
            self.assertIn(marker,src)


if __name__=='__main__':
    unittest.main()
