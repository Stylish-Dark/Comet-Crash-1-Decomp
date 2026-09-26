from __future__ import annotations
import argparse
from pathlib import Path

MARKER = '# COMET_INLINE_JT_FALLBACK'
ANCHOR = '''        _dbg(all_insns[i].addr, f"bases={_bases} toc={toc}")
        if not _bases or not toc:
            continue
'''
PATCH = '''        _dbg(all_insns[i].addr, f"bases={_bases} toc={toc}")
        # COMET_INLINE_JT_FALLBACK
        # Some SN/GCC switches spill the table base to the stack long before
        # the dispatcher, so the backward register-definition walk cannot
        # resolve a TOC load. When the bctr is immediately followed by signed
        # relative offsets and the loaded/extended entry is added into CTR, the
        # next word is itself the table base. Decode that structure directly.
        # Otherwise the runtime indirect dispatcher sees interior case labels as
        # unresolved calls because it only registers function entry addresses.
        if not _bases:
            _inline_rel = False
            for _w in win:
                if _w.mnemonic != 'add':
                    continue
                _a = [x.strip() for x in _w.operands.split(',')]
                if len(_a) == 3 and _a[0] == rC and (_a[1] in _off or _a[2] in _off):
                    _inline_rel = True
                    break
            if _inline_rel:
                _base = (all_insns[i].addr + 4) & 0xFFFFFFFF
                _targets = []
                for _k in range(256):
                    _ea = (_base + _k * 4) & 0xFFFFFFFF
                    _fwd = [t for t in _targets if t > _base]
                    if _fwd and _ea >= min(_fwd):
                        break
                    _v = read_u32(_ea)
                    if _v is None:
                        break
                    _off32 = _v - (1 << 32) if (_v & 0x80000000) else _v
                    _t = (_base + _off32) & 0xFFFFFFFF
                    if text_lo <= _t < text_hi and _t % 4 == 0:
                        _targets.append(_t)
                    elif _targets or _k >= 4:
                        break
                if len(_targets) >= 2:
                    tables[all_insns[i].addr] = sorted(set(_targets))
                    _dbg(all_insns[i].addr, f"inline-relative base=0x{_base:X} decoded {len(_targets)} targets")
                    continue
        if not _bases or not toc:
            continue
'''

def patch_text(src: str) -> tuple[str, bool]:
    if MARKER in src:
        return src, False
    n = src.count(ANCHOR)
    if n != 1:
        raise ValueError(f'expected one ps3recomp jump-table anchor, found {n}')
    return src.replace(ANCHOR, PATCH, 1), True

def patch_file(path: Path) -> bool:
    src = path.read_text(encoding='utf-8')
    out, changed = patch_text(src)
    if changed:
        path.write_text(out, encoding='utf-8')
    return changed

def main() -> int:
    ap = argparse.ArgumentParser(description='Patch pinned ps3recomp for stack-spilled inline relative jump tables')
    ap.add_argument('ppu_lifter', type=Path)
    a = ap.parse_args()
    print('Comet inline jump-table lifter patch:', 'applied' if patch_file(a.ppu_lifter) else 'already present')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
