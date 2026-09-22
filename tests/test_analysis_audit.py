import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import audit_analysis as a


class AnalysisAuditTests(unittest.TestCase):
    def _fixture(self, root: Path):
        spu = root / 'spu'; spu.mkdir()
        first = spu / 'spu_0000.elf'; first.write_bytes(b'abc')
        second = spu / 'spu_0001.elf'; second.write_bytes(b'defgh')
        h = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
        baseline = {
            'ppu_opd': {
                'descriptor_count': 3,
                'unique_function_count': 2,
                'entry_code': '0x100',
                'module_toc': '0x200',
            },
            'firmware_imports': {
                'count': 3,
                'library_count': 2,
                'by_library': {'a': 2, 'b': 1},
            },
            'spu_images': [
                {'size': 3, 'sha256': h(first)},
                {'size': 5, 'sha256': h(second)},
            ],
        }
        (root / 'baseline.json').write_text(json.dumps(baseline))
        loader = {'opd_count': 3, 'function_count': 2, 'entry_code': '0x100', 'module_toc': '0x200'}
        (root / 'loader.json').write_text(json.dumps(loader))
        imports = [{'library': 'a'}, {'library': 'b'}, {'library': 'a'}]
        (root / 'imports.json').write_text(json.dumps(imports))
        return root / 'loader.json', root / 'imports.json', spu, root / 'baseline.json'

    def test_exact_baseline_passes(self):
        with tempfile.TemporaryDirectory() as td:
            args = self._fixture(Path(td))
            r = a.audit_analysis(*args)
            self.assertTrue(r['ok'], r['errors'])
            self.assertEqual(r['imports'], 3)
            self.assertEqual(r['spu_images'], 2)

    def test_import_or_spu_drift_fails(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td); loader, imports, spu, baseline = self._fixture(root)
            data = json.loads(imports.read_text()); data.pop(); imports.write_text(json.dumps(data))
            (spu / 'unexpected.elf').write_bytes(b'x')
            r = a.audit_analysis(loader, imports, spu, baseline)
            self.assertFalse(r['ok'])
            self.assertTrue(any('import count' in x for x in r['errors']))
            self.assertTrue(any('SPU image set mismatch' in x for x in r['errors']))

    def test_ppu_address_or_count_drift_fails(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td); loader, imports, spu, baseline = self._fixture(root)
            data = json.loads(loader.read_text()); data['function_count'] = 99; data['entry_code'] = '0x104'; loader.write_text(json.dumps(data))
            r = a.audit_analysis(loader, imports, spu, baseline)
            self.assertFalse(r['ok'])
            self.assertTrue(any('function_count' in x for x in r['errors']))
            self.assertTrue(any('entry_code' in x for x in r['errors']))


if __name__ == '__main__':
    unittest.main()
