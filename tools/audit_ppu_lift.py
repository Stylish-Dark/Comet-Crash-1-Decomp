from __future__ import annotations

import argparse
import hashlib
import re
from pathlib import Path

TODO_RE = re.compile(r'/\*\s*TODO:\s*(.*?)\s*\*/;')
UNSUPPORTED_SPR_RE = re.compile(r'/\*\s*(.*?)\s*:\s*unsupported SPR -- no-op\s*\*/;')
RAW_WORD_RE = re.compile(r'^\.word 0x([0-9A-Fa-f]{8})$')
JUMP_SWITCH_RE = re.compile(r'switch \(\(uint32_t\)ctx->ctr\) \{(.*?)default: ps3_indirect_call\(ctx\); return; \} return;', re.DOTALL)
JUMP_CASE_RE = re.compile(r'case 0x([0-9A-Fa-f]{8})u:')


def audit_text(src: str) -> list[dict[str, object]]:
    """Return every explicit unimplemented/no-op lifter hole in generated PPU C++."""
    holes: list[dict[str, object]] = []
    for kind, pattern in (('todo', TODO_RE), ('unsupported-spr-noop', UNSUPPORTED_SPR_RE)):
        for match in pattern.finditer(src):
            instruction=match.group(1).strip()
            if kind == 'todo' and RAW_WORD_RE.fullmatch(instruction):
                continue
            holes.append({
                'line': src.count('\n', 0, match.start()) + 1,
                'instruction': instruction,
                'kind': kind,
                'text': match.group(0),
                '_offset': match.start(),
            })
    holes.sort(key=lambda item: int(item['_offset']))
    for item in holes:
        item.pop('_offset', None)
    return holes


def raw_word_values(src: str) -> list[str]:
    values=[]
    for match in TODO_RE.finditer(src):
        raw=RAW_WORD_RE.fullmatch(match.group(1).strip())
        if raw:
            values.append(raw.group(1).lower())
    return values


def raw_word_digest(values: list[str]) -> str:
    payload=('\n'.join(values)+('\n' if values else '')).encode('ascii')
    return hashlib.sha256(payload).hexdigest()


def jump_table_blocks(src: str) -> list[list[str]]:
    """Return ordered generated computed-switch case targets.

    ps3recomp emits recovered PPU jump tables as computed switches over
    ctx->ctr. Keeping block boundaries in the digest detects a regression that
    merges/splits tables or moves a case between dispatchers even if the
    aggregate target set is unchanged.
    """
    return [[v.lower() for v in JUMP_CASE_RE.findall(match.group(1))]
            for match in JUMP_SWITCH_RE.finditer(src)]


def jump_table_digest(blocks: list[list[str]]) -> str:
    payload=''.join(','.join(values)+'\n' for values in blocks).encode('ascii')
    return hashlib.sha256(payload).hexdigest()


def generated_sources(path: Path) -> list[Path]:
    if path.is_file():
        return [path]
    return sorted(path.glob('ppu_recomp_*.cpp'))


def audit_path(path: Path) -> dict[str, object]:
    sources = generated_sources(path)
    if not sources:
        raise FileNotFoundError(f'no generated PPU chunks found under {path}')

    holes: list[dict[str, object]] = []
    raw_words: list[str] = []
    jump_blocks: list[list[str]] = []
    for source in sources:
        # ps3recomp writes generated source using the host default encoding.
        # The audit patterns are ASCII-only, so Latin-1 gives a reversible
        # one-byte mapping on UTF-8, CP-1252 and other single-byte output.
        src=source.read_bytes().decode('latin-1')
        for item in audit_text(src):
            holes.append({'file': str(source), **item})
        raw_words.extend(raw_word_values(src))
        jump_blocks.extend(jump_table_blocks(src))
    return {
        'source_files': len(sources),
        'unsupported_total': len(holes),
        'unsupported': holes,
        'raw_word_total': len(raw_words),
        'raw_word_sha256': raw_word_digest(raw_words),
        'jump_table_dispatchers': len(jump_blocks),
        'jump_table_case_occurrences': sum(len(block) for block in jump_blocks),
        'jump_table_unique_targets': len({v for block in jump_blocks for v in block}),
        'jump_table_sha256': jump_table_digest(jump_blocks),
    }


def main() -> int:
    ap = argparse.ArgumentParser(description='Fail on explicit unimplemented/no-op instructions in a generated ps3recomp PPU lift')
    ap.add_argument('path', type=Path)
    a = ap.parse_args()
    report = audit_path(a.path)
    print(f'PPU audit: files={report["source_files"]}, unsupported={report["unsupported_total"]}, raw-word={report["raw_word_total"]} sha256={report["raw_word_sha256"]}, jump-tables={report["jump_table_dispatchers"]}/{report["jump_table_case_occurrences"]} sha256={report["jump_table_sha256"]}')
    for item in report['unsupported']:
        print(f'  {item["file"]}:{item["line"]}: {item["instruction"]}')
    return 1 if report['unsupported_total'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
