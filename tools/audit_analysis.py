from __future__ import annotations

import argparse
import hashlib
import json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BASELINE = ROOT / 'docs' / 'reference' / 'known-analysis.json'


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def _int_value(value: object) -> int:
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        return int(value, 0)
    raise TypeError(f'expected integer/hex string, got {type(value).__name__}')


def audit_analysis(loader_path: Path, imports_path: Path, spu_dir: Path,
                   baseline_path: Path = DEFAULT_BASELINE) -> dict[str, object]:
    baseline = json.loads(baseline_path.read_text(encoding='utf-8'))
    loader = json.loads(loader_path.read_text(encoding='utf-8'))
    imports = json.loads(imports_path.read_text(encoding='utf-8'))

    errors: list[str] = []
    ppu = baseline['ppu_opd']
    expected_loader = {
        'opd_count': int(ppu['descriptor_count']),
        'function_count': int(ppu['unique_function_count']),
        'entry_code': _int_value(ppu['entry_code']),
        'module_toc': _int_value(ppu['module_toc']),
    }
    actual_loader = {
        'opd_count': int(loader.get('opd_count', -1)),
        'function_count': int(loader.get('function_count', -1)),
        'entry_code': _int_value(loader.get('entry_code', '-1')),
        'module_toc': _int_value(loader.get('module_toc', '-1')),
    }
    for key, expected in expected_loader.items():
        actual = actual_loader[key]
        if actual != expected:
            errors.append(f'PPU {key}: got {actual:#x} expected {expected:#x}' if key in ('entry_code', 'module_toc')
                          else f'PPU {key}: got {actual} expected {expected}')

    expected_imports = baseline['firmware_imports']
    actual_by_lib = Counter(str(item.get('library', '')) for item in imports)
    expected_by_lib = {str(k): int(v) for k, v in expected_imports['by_library'].items()}
    if len(imports) != int(expected_imports['count']):
        errors.append(f'import count: got {len(imports)} expected {expected_imports["count"]}')
    if len(actual_by_lib) != int(expected_imports['library_count']):
        errors.append(f'import library count: got {len(actual_by_lib)} expected {expected_imports["library_count"]}')
    if dict(sorted(actual_by_lib.items())) != dict(sorted(expected_by_lib.items())):
        errors.append(f'import distribution mismatch: got {dict(sorted(actual_by_lib.items()))} expected {dict(sorted(expected_by_lib.items()))}')

    expected_spu = sorted(
        (int(item['size']), str(item['sha256']).lower())
        for item in baseline['spu_images']
    )
    actual_spu = sorted(
        (path.stat().st_size, _sha256(path))
        for path in spu_dir.glob('*.elf')
    )
    if actual_spu != expected_spu:
        errors.append(f'SPU image set mismatch: got {actual_spu} expected {expected_spu}')

    return {
        'ok': not errors,
        'errors': errors,
        'ppu': actual_loader,
        'imports': len(imports),
        'libraries': len(actual_by_lib),
        'spu_images': len(actual_spu),
    }


def main() -> int:
    ap = argparse.ArgumentParser(description='Verify ps3recomp analysis against the pinned Comet Crash baseline')
    ap.add_argument('loader', type=Path)
    ap.add_argument('imports', type=Path)
    ap.add_argument('spu_dir', type=Path)
    ap.add_argument('--baseline', type=Path, default=DEFAULT_BASELINE)
    a = ap.parse_args()
    report = audit_analysis(a.loader, a.imports, a.spu_dir, a.baseline)
    print(f'analysis baseline: ppu={report["ppu"]["function_count"]} funcs, imports={report["imports"]}/{report["libraries"]} libs, spu={report["spu_images"]}')
    for error in report['errors']:
        print(f'  ERROR: {error}')
    return 0 if report['ok'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
