from __future__ import annotations
import argparse, re
from pathlib import Path

TODO_RE = re.compile(r'/\* TODO: (vsrab|vsrb) v(\d+), v(\d+), v(\d+) \*/;')

def _replacement(m: re.Match[str]) -> str:
    mn, vd, va, vb = m.group(1), *map(int, m.groups()[1:])
    if mn == 'vsrb':
        return (f'{{ uint8_t _a[16], _b[16], _d[16]; '
                f'memcpy(_a, &ctx->vr[{va}], 16); memcpy(_b, &ctx->vr[{vb}], 16); '
                f'for (int _i=0; _i<16; ++_i) _d[_i]=(uint8_t)(_a[_i] >> (_b[_i] & 7u)); '
                f'memcpy(&ctx->vr[{vd}], _d, 16); }}')
    return (f'{{ uint8_t _a[16], _b[16], _d[16]; '
            f'memcpy(_a, &ctx->vr[{va}], 16); memcpy(_b, &ctx->vr[{vb}], 16); '
            f'for (int _i=0; _i<16; ++_i) _d[_i]=(uint8_t)((int8_t)_a[_i] >> (_b[_i] & 7u)); '
            f'memcpy(&ctx->vr[{vd}], _d, 16); }}')

def patch_text(src: str):
    stats = {'vsrab': 0, 'vsrb': 0}
    def repl(m):
        stats[m.group(1)] += 1
        return _replacement(m)
    return TODO_RE.sub(repl, src), stats

def patch_file(path: Path):
    src = path.read_text(encoding='utf-8')
    out, stats = patch_text(src)
    if out != src:
        path.write_text(out, encoding='utf-8', newline='\n')
    return stats

def main() -> int:
    ap = argparse.ArgumentParser(description='Patch Comet Crash PPU lift holes not handled by the pinned ps3recomp lifter')
    ap.add_argument('paths', nargs='+', type=Path)
    args = ap.parse_args()
    total={'vsrab':0,'vsrb':0}
    files=0
    for p in args.paths:
        targets=sorted(p.glob('ppu_recomp_*.cpp')) if p.is_dir() else [p]
        for f in targets:
            st=patch_file(f)
            if any(st.values()): files += 1
            for k,v in st.items(): total[k]+=v
    print(f'patched {files} file(s): vsrab={total["vsrab"]}, vsrb={total["vsrb"]}')
    return 0
if __name__=='__main__': raise SystemExit(main())
