import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import comet_port
import verify_boot_bundle as v


class BootBundleTests(unittest.TestCase):
    def _make_bundle(self, root: Path, *, provenance_status="verified"):
        log=root/'boot.txt'
        metadata={
            'elf_sha256':'a'*64,
            'build_provenance':{
                'schema_version':2,
                'ps3recomp_commit':'b'*40,
                'port_git':{'commit':'c'*40,'dirty':False,'tracked_diff_sha256':'d'*64},
                'hle_coverage':{'imports':171,'covered_imports':171,'missing':0},
                'generated_ppu_chunks':1,
                'generated_spu_units':2,
                'native_executable':{'name':'CometCrashPC.exe','size':123,'sha256':'e'*64},
            },
            'provenance_verification':{'status':provenance_status},
        }
        code=(
            'print("[boot-stage] entering recompiled title"); '
            'print("[HLE] UNIMPLEMENTED nid=0x12345678"); '
            'raise SystemExit(9)'
        )
        with self.assertRaises(subprocess.CalledProcessError):
            comet_port.run_logged([sys.executable,'-c',code],log,metadata=metadata)
        return log.with_suffix('.summary.json'),log

    def test_valid_bundle_rederives_log_evidence(self):
        with tempfile.TemporaryDirectory() as td:
            summary,log=self._make_bundle(Path(td))
            report=v.verify_bundle(summary)
            self.assertTrue(report['ok'],report['errors'])
            self.assertEqual(report['boot_outcome'],'failure-before-frame')
            self.assertEqual(report['suspected_subsystem'],'hle')
            data=json.loads(summary.read_text(encoding='utf-8'))
            self.assertEqual(data['summary_schema_version'],1)
            self.assertEqual(data['boot_log_sha256'],comet_port.sha256_file(log))

    def test_tampered_text_log_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            summary,log=self._make_bundle(Path(td))
            with log.open('a',encoding='utf-8') as f:
                f.write('tampered\n')
            report=v.verify_bundle(summary)
            self.assertFalse(report['ok'])
            self.assertTrue(any('SHA-256 mismatch' in x for x in report['errors']))

    def test_tampered_summary_outcome_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            summary,_=self._make_bundle(Path(td))
            data=json.loads(summary.read_text(encoding='utf-8'))
            data['boot_outcome']='clean-visible-exit'
            summary.write_text(json.dumps(data),encoding='utf-8')
            report=v.verify_bundle(summary)
            self.assertFalse(report['ok'])
            self.assertTrue(any('boot_outcome mismatch' in x for x in report['errors']))

    def test_tampered_memalign_origin_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            summary,log=self._make_bundle(Path(td))
            with log.open('a',encoding='utf-8') as f:
                f.write('[COMET-MEMALIGN-CORE-LOW] result=0x00000140 least=0x40000000\n')
            data=json.loads(summary.read_text(encoding='utf-8'))
            data['boot_log_sha256']=comet_port.sha256_file(log)
            data['memalign_origin']='wrapper-return'
            summary.write_text(json.dumps(data),encoding='utf-8')
            report=v.verify_bundle(summary)
            self.assertFalse(report['ok'])
            self.assertTrue(any('memalign_origin mismatch' in x for x in report['errors']))

    def test_tampered_malloc_source_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            summary,log=self._make_bundle(Path(td))
            with log.open('a',encoding='utf-8') as f:
                f.write('[COMET-MALLOC-LOW] source=loc_001A61A8#1 request=0x390 result=0x140\n')
            data=json.loads(summary.read_text(encoding='utf-8'))
            data['boot_log_sha256']=comet_port.sha256_file(log)
            data['malloc_source']='loc_DEADBEEF#1'
            summary.write_text(json.dumps(data),encoding='utf-8')
            report=v.verify_bundle(summary)
            self.assertFalse(report['ok'])
            self.assertTrue(any('malloc_source mismatch' in x for x in report['errors']))

    def test_tampered_parse_corruption_site_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            summary,log=self._make_bundle(Path(td))
            with log.open('a',encoding='utf-8') as f:
                f.write('[COMET-PARSE-SLOT-CHANGE] site=0x00130204 callee=0x0001C7D4 expected=0x43C81280 got=0x00000140\n')
            data=json.loads(summary.read_text(encoding='utf-8'))
            data['boot_log_sha256']=comet_port.sha256_file(log)
            data['parse_corruption_kind']='slot-change'
            data['parse_corruption_site']='0XDEADBEEF'
            summary.write_text(json.dumps(data),encoding='utf-8')
            report=v.verify_bundle(summary)
            self.assertFalse(report['ok'])
            self.assertTrue(any('parse_corruption_site mismatch' in x for x in report['errors']))

    def test_tampered_parse_writer_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            summary,log=self._make_bundle(Path(td))
            with log.open('a',encoding='utf-8') as f:
                f.write('[COMET-PARSE-WRITE] slot=0xD0071864 addr=0xD0071864 value=0x00000140 width=4 expected=0x43C81280 before=0x43C81280 guest_fn=0x0019ABCD\n')
            data=json.loads(summary.read_text(encoding='utf-8'))
            data['boot_log_sha256']=comet_port.sha256_file(log)
            data['parse_write_kind']='guest-write'
            data['parse_write_function']='0XDEADBEEF'
            summary.write_text(json.dumps(data),encoding='utf-8')
            report=v.verify_bundle(summary)
            self.assertFalse(report['ok'])
            self.assertTrue(any('parse_write_function mismatch' in x for x in report['errors']))

    def test_unverified_provenance_requires_explicit_acceptance(self):
        with tempfile.TemporaryDirectory() as td:
            summary,_=self._make_bundle(Path(td),provenance_status='overridden')
            strict=v.verify_bundle(summary)
            self.assertFalse(strict['ok'])
            relaxed=v.verify_bundle(summary,require_verified_provenance=False)
            self.assertTrue(relaxed['ok'],relaxed['errors'])

    def test_adjacent_log_name_is_deterministic(self):
        self.assertEqual(
            v.adjacent_log_path(Path('logs/boot-1.summary.json')),
            Path('logs/boot-1.txt'),
        )
        with self.assertRaises(ValueError):
            v.adjacent_log_path(Path('logs/boot-1.json'))


if __name__=='__main__':
    unittest.main()
