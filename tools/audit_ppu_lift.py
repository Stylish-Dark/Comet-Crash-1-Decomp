from __future__ import annotations

import argparse
import re
from pathlib import Path

TODO_RE = re.compile(r'/\*\s*TODO:\s*(.*?)\s*\*/;')
UNSUPPORTED_SPR_RE = re.compile(r'/\*\s*(.*?)\s*:\s*unsupported SPR -- no-op\s*\*/;')


def audit_text(src: str) -> list[dict[str, object]]:
    """Return every explicit unimplemented/no-op lifter hole in generated PPU C++."""
    holes: list[dict[str, object]] = []
    for kind, pattern in (('todo', TODO_RE), ('unsupported-spr-noop', UNSUPPORTED_SPR_RE)):
        for match in pattern.finditer(src):
            holes.append({
                'line': src.count('\n', 0, match.start()) + 1,
                'instruction': match.group(1).strip(),
                'kind': kind,
                'text': match.group(0),
                '_offset': match.start(),
            })
    holes.sort(key=lambda item: int(item['_offset']))
    for item in holes:
        item.pop('_offset', None)
    return holes


def generated_sources(path: Path) -> list[Path]:
    if path.is_file():
        return [path]
    return sorted(path.glob('ppu_recomp_*.cpp'))


def audit_path(path: Path) -> dict[str, object]:
    sources = generated_sources(path)
    if not sources:
        raise FileNotFoundError(f'no generated PPU chunks found under {path}')

    holes: list[dict[str, object]] = []
    for source in sources:
        for item in audit_text(source.read_text(encoding='utf-8')):
            holes.append({'file': str(source), **item})
    return {
        'source_files': len(sources),
        'unsupported_total': len(holes),
        'unsupported': holes,
    }


def main() -> int:
    ap = argparse.ArgumentParser(description='Fail on explicit unimplemented/no-op instructions in a generated ps3recomp PPU lift')
    ap.add_argument('path', type=Path)
    a = ap.parse_args()
    report = audit_path(a.path)
    print(f'PPU audit: files={report["source_files"]}, unsupported={report["unsupported_total"]}')
    for item in report['unsupported']:
        print(f'  {item["file"]}:{item["line"]}: {item["instruction"]}')
    return 1 if report['unsupported_total'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
