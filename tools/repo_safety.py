from __future__ import annotations
import subprocess, sys
from pathlib import PurePosixPath

FORBIDDEN_NAMES = {"eboot.bin","eboot.elf","param.sfo","param.pfd","lic.dat"}
FORBIDDEN_SUFFIXES = {".pkg",".rap",".rif",".self",".sprx",".pak",".edat",".sdat",".psarc"}
FORBIDDEN_ROOTS = {"game","gamefiles","input-game","proprietary"}

def is_forbidden(path: str) -> bool:
    p = PurePosixPath(path.replace("\\", "/"))
    parts = [x.lower() for x in p.parts]
    if parts and parts[0] in FORBIDDEN_ROOTS:
        return True
    if p.name.lower() in FORBIDDEN_NAMES:
        return True
    return p.suffix.lower() in FORBIDDEN_SUFFIXES

def tracked_files() -> list[str]:
    raw = subprocess.check_output(["git","ls-files","-z"])
    return [x.decode("utf-8", "surrogateescape") for x in raw.split(b"\0") if x]

def main() -> int:
    bad = [p for p in tracked_files() if is_forbidden(p)]
    if bad:
        print("ERROR: proprietary/forbidden files are tracked:", file=sys.stderr)
        for p in bad: print(f"  {p}", file=sys.stderr)
        return 1
    print("repository safety: OK")
    return 0
if __name__ == "__main__": raise SystemExit(main())
