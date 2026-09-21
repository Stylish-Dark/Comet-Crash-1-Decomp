from __future__ import annotations
import argparse
from pathlib import Path

OLD = '''    /* Fill with simple linear interpolation weights */
    float* table = (float*)buf;
    for (u32 i = 0; i < tableLen; i++)
        table[i] = (float)i / (float)(tableLen > 1 ? tableLen - 1 : 1);
'''

NEW = '''    /* `buf` is a PS3 guest EA, not a host pointer.  Write each float through
     * the guest-memory helper so the table lands in vm_base and is stored in
     * the big-endian byte order the lifted PPU expects. */
    const u32 table_ea = (u32)(uintptr_t)buf;
    for (u32 i = 0; i < tableLen; i++) {
        const float weight = (float)i / (float)(tableLen > 1 ? tableLen - 1 : 1);
        u32 bits;
        memcpy(&bits, &weight, sizeof(bits));
        vm_write32(table_ea + i * 4u, bits);
    }
'''


def patch_text(src: str) -> tuple[str, bool]:
    if 'const u32 table_ea = (u32)(uintptr_t)buf;' in src and 'vm_write32(table_ea + i * 4u, bits);' in src:
        return src, False
    if 's32 cellRescCreateInterlaceTable' not in src or OLD not in src:
        raise ValueError('expected ps3recomp cellRescCreateInterlaceTable markers not found; upstream changed')
    return src.replace(OLD, NEW, 1), True


def patch_file(path: Path) -> bool:
    src = path.read_text(encoding='utf-8')
    out, changed = patch_text(src)
    if changed:
        path.write_text(out, encoding='utf-8', newline='\n')
    return changed


def patch_checkout(root: Path) -> bool:
    return patch_file(root / 'libs' / 'video' / 'cellResc.c')


def main() -> int:
    ap = argparse.ArgumentParser(description='Fix ps3recomp RESC guest-memory interlace table writes for Comet Crash')
    ap.add_argument('ps3recomp', type=Path)
    a = ap.parse_args()
    p = a.ps3recomp / 'libs' / 'video' / 'cellResc.c'
    changed = patch_file(p)
    print(f'{p}: {"patched" if changed else "already patched"}')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
