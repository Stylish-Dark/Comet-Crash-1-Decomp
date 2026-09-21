from __future__ import annotations
import argparse, os, shutil, subprocess, sys
from pathlib import Path

def version(cmd):
    try: return subprocess.check_output(cmd, text=True, stderr=subprocess.STDOUT).splitlines()[0]
    except Exception: return None

def find_ninja() -> str|None:
    """Find Ninja either on PATH or inside the PyPI ``ninja`` wheel."""
    found=shutil.which("ninja")
    if found: return found
    try:
        import ninja
    except ImportError:
        return None
    bin_dir=getattr(ninja,"BIN_DIR",None)
    if not bin_dir: return None
    exe=Path(bin_dir)/("ninja.exe" if os.name=="nt" else "ninja")
    return str(exe) if exe.is_file() else None

def main() -> int:
    ap=argparse.ArgumentParser(); ap.add_argument("--ps3recomp",type=Path); a=ap.parse_args()
    required={"git":"git","cmake":"cmake","clang-cl":"clang-cl"}
    missing=[]
    for label,exe in required.items():
        p=shutil.which(exe); print(f"{label}: {p or 'MISSING'}")
        if not p: missing.append(label)
    ninja=find_ninja(); print(f"ninja: {ninja or 'MISSING'}")
    if not ninja: missing.append("ninja")
    print(f"python: {sys.version.split()[0]}")
    if a.ps3recomp:
        good=(a.ps3recomp/"tools"/"ppu_lifter.py").exists()
        print(f"ps3recomp: {'OK' if good else 'MISSING/INVALID'} {a.ps3recomp}")
        if not good: missing.append("ps3recomp")
    return 1 if missing else 0
if __name__ == "__main__": raise SystemExit(main())
