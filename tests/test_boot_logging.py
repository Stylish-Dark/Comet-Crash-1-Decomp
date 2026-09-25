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


    def test_summary_tracks_last_stage_and_first_signal(self):
        with tempfile.TemporaryDirectory() as td:
            log=Path(td)/'boot.txt'
            code=('import sys; '
                  'print("[boot-stage] process entered"); '
                  'print("[boot-stage] entering recompiled title"); '
                  'print("[watchdog] no guest frame after 10s"); '
                  'print("[crash] code=0xC0000005"); '
                  'raise SystemExit(9)')
            with self.assertRaises(subprocess.CalledProcessError):
                comet_port.run_logged([sys.executable,'-c',code],log)
            text=log.read_text(encoding='utf-8')
            self.assertIn('# last_boot_stage=entering recompiled title',text)
            self.assertIn('# first_signal=[watchdog] no guest frame after 10s',text)
            self.assertIn('# triage_signal=[watchdog] no guest frame after 10s',text)
            self.assertIn('# suspected_subsystem=unknown',text)
            self.assertIn('watchdog confirms a hang but does not identify its subsystem',text)
            self.assertIn('# host_exit_code=9',text)

    def test_later_specific_signal_drives_live_triage(self):
        with tempfile.TemporaryDirectory() as td:
            log=Path(td)/'boot.txt'
            code=('print("[boot-stage] entering recompiled title"); '
                  'print("[watchdog] no guest frame after 10s"); '
                  'print("[HLE] UNIMPLEMENTED nid=0xCAFEBABE"); '
                  'raise SystemExit(9)')
            with self.assertRaises(subprocess.CalledProcessError):
                comet_port.run_logged([sys.executable,'-c',code],log)
            text=log.read_text(encoding='utf-8')
            self.assertIn('# first_signal=[watchdog] no guest frame after 10s',text)
            self.assertIn('# triage_signal=[HLE] UNIMPLEMENTED nid=0xCAFEBABE',text)
            self.assertIn('# suspected_subsystem=hle',text)

    def test_run_logged_records_memalign_origin(self):
        with tempfile.TemporaryDirectory() as td:
            log=Path(td)/'boot.txt'
            code=(
                'print("[COMET-MEMALIGN-MALLOC-LOW] result=0x00000140 least=0x40000000"); '
                'print("[COMET-MEMALIGN-CORE-LOW] result=0x00000140 least=0x40000000"); '
                'print("[COMET-MEMALIGN-WRAPPER-LOW] result=0x00000140 least=0x40000000"); '
                'raise SystemExit(9)'
            )
            with self.assertRaises(subprocess.CalledProcessError):
                comet_port.run_logged([sys.executable,'-c',code],log)
            text=log.read_text(encoding='utf-8')
            self.assertIn('# memalign_origin=backing-malloc',text)
            self.assertIn('# memalign_signal=[COMET-MEMALIGN-MALLOC-LOW]',text)
            data=json.loads(log.with_suffix('.summary.json').read_text(encoding='utf-8'))
            self.assertEqual(data['memalign_origin'],'backing-malloc')
            self.assertIn('MEMALIGN-MALLOC-LOW',data['memalign_signal'])
            self.assertEqual(data['suspected_subsystem'],'vm/ppu')

    def test_run_logged_classifies_hle_failure(self):
        with tempfile.TemporaryDirectory() as td:
            log=Path(td)/'boot.txt'
            code=('print("[boot-stage] entering recompiled title"); '
                  'print("[HLE] UNIMPLEMENTED nid=0x12345678"); '
                  'raise SystemExit(9)')
            with self.assertRaises(subprocess.CalledProcessError):
                comet_port.run_logged([sys.executable,'-c',code],log)
            text=log.read_text(encoding='utf-8')
            self.assertIn('# suspected_subsystem=hle',text)
            self.assertIn('# boot_outcome=failure-before-frame',text)
            self.assertIn("matched '[hle] unimplemented'",text.lower())

    def test_success_sidecar_records_first_frame(self):
        with tempfile.TemporaryDirectory() as td:
            log=Path(td)/'boot.txt'
            code=('print("[boot-stage] entering recompiled title"); '
                  'print("[boot-stage] first guest frame presented")')
            comet_port.run_logged([sys.executable,'-c',code],log)
            data=json.loads(log.with_suffix('.summary.json').read_text(encoding='utf-8'))
            self.assertTrue(data['first_frame_presented'])
            self.assertEqual(data['boot_outcome'],'clean-visible-exit')
            self.assertEqual(data['host_exit_code'],0)

    def test_timeout_uses_escalating_stop_helper(self):
        src=(ROOT/'tools'/'comet_port.py').read_text(encoding='utf-8')
        start=src.index('def timeout_proc()')
        end=src.index('timer=threading.Timer',start)
        self.assertIn('_stop_process(proc)',src[start:end])

    def test_run_logged_timeout_preserves_diagnostics(self):
        with tempfile.TemporaryDirectory() as td:
            log=Path(td)/'boot.txt'
            code=('import time; '
                  'print("[boot-stage] entering recompiled title", flush=True); '
                  'time.sleep(30)')
            with self.assertRaises(subprocess.TimeoutExpired):
                comet_port.run_logged(
                    [sys.executable,'-c',code],
                    log,
                    timeout_seconds=0.2,
                )
            text=log.read_text(encoding='utf-8')
            self.assertIn('# last_boot_stage=entering recompiled title',text)
            self.assertIn('# timed_out=true',text)
            self.assertIn('# interrupted=<none>',text)
            self.assertIn('# boot_outcome=timed-out',text)
            self.assertIn('# host_exit_code=',text)
            sidecar=log.with_suffix('.summary.json')
            self.assertTrue(sidecar.is_file())
            data=json.loads(sidecar.read_text(encoding='utf-8'))
            self.assertEqual(data['boot_outcome'],'timed-out')
            self.assertEqual(data['last_boot_stage'],'entering recompiled title')

    def test_run_logged_rejects_nonpositive_timeout(self):
        with tempfile.TemporaryDirectory() as td:
            with self.assertRaisesRegex(ValueError,'must be > 0'):
                comet_port.run_logged(
                    [sys.executable,'-c','pass'],
                    Path(td)/'boot.txt',
                    timeout_seconds=0,
                )

    def test_interrupt_path_is_durable_by_construction(self):
        src=(ROOT/'tools'/'comet_port.py').read_text(encoding='utf-8')
        self.assertIn('except KeyboardInterrupt:',src)
        self.assertIn("interrupted='keyboard'",src)
        self.assertIn('# interrupted=',src)
        self.assertIn('_stop_process(proc)',src)

    def test_run_parser_accepts_explicit_log_path(self):
        args=comet_port.parser().parse_args(['run','game','EBOOT.ELF','--log','logs/custom.txt'])
        self.assertEqual(args.log,Path('logs/custom.txt'))
        args=comet_port.parser().parse_args(['run','game','EBOOT.ELF','--timeout','45'])
        self.assertEqual(args.timeout,45.0)

    def test_native_crash_records_runtime_context(self):
        src=(ROOT/'port'/'main.cpp').read_text(encoding='utf-8')
        self.assertIn('[crash] last_stage=%s frames=%ld last_hle=',src)
        self.assertIn('access_target_region=guest_vm',src)
        self.assertIn('guest_offset=0x%llX',src)
        self.assertIn('access_target_region=outside_guest_vm',src)

    def test_watchdog_has_capped_long_hang_snapshots(self):
        src=(ROOT/'port'/'main.cpp').read_text(encoding='utf-8')
        self.assertIn('10000u,20000u,30000u,60000u,180000u',src)
        self.assertIn('Cumulative snapshots at 10s, 30s, 60s, 120s and 300s',src)

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
