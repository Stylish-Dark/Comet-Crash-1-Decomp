from __future__ import annotations
import argparse
from pathlib import Path

OLD='s_d3d.swap_chain->lpVtbl->Present(s_d3d.swap_chain, 1, 0); /* vsync */'
NEW='''{ const char* comet_vsync = getenv("COMET_VSYNC");\n        UINT comet_sync_interval = (!comet_vsync || comet_vsync[0] != '0') ? 1u : 0u;\n        s_d3d.swap_chain->lpVtbl->Present(s_d3d.swap_chain, comet_sync_interval, 0); } /* Comet host VSync */'''

def patch_text(s: str) -> tuple[str,bool]:
    if 'comet_sync_interval' in s:
        return s, False
    if OLD not in s:
        raise ValueError('expected ps3recomp D3D12 Present(1,0) site not found; upstream changed')
    if s.count(OLD) != 1:
        raise ValueError(f'expected one D3D12 Present site, found {s.count(OLD)}')
    return s.replace(OLD,NEW), True

def patch_file(path: Path) -> bool:
    src=path.read_text(encoding='utf-8')
    out,changed=patch_text(src)
    if changed:
        path.write_text(out,encoding='utf-8',newline='\n')
    return changed

def main()->int:
    ap=argparse.ArgumentParser(description='Apply Comet Crash host hooks to ps3recomp D3D12 source')
    ap.add_argument('ps3recomp',type=Path)
    a=ap.parse_args()
    p=a.ps3recomp/'libs'/'video'/'rsx_d3d12_backend.c'
    changed=patch_file(p)
    print(f'{p}: {"patched" if changed else "already patched"}')
    return 0
if __name__=='__main__': raise SystemExit(main())
