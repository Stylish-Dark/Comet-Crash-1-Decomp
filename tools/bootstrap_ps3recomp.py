from __future__ import annotations
import argparse, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOCK = ROOT / "config" / "ps3recomp.lock"

def load_lock(path: Path = LOCK) -> dict[str,str]:
    out = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        line=line.strip()
        if line and not line.startswith("#"):
            k,v=line.split("=",1); out[k.strip()]=v.strip()
    if not out.get("repository") or not out.get("commit"):
        raise ValueError("ps3recomp.lock needs repository and commit")
    return out

def build_bootstrap_commands(checkout: Path, lock: dict[str,str]) -> list[list[str]]:
    if checkout.exists() and (checkout/".git").exists():
        # external/ps3recomp is a reproducible toolchain cache, not a workspace.
        # Reset it completely before every pipeline run so an interrupted prior
        # Comet runtime patch cannot leak into the next lift/build.
        return [["git","-C",str(checkout),"remote","set-url","origin",lock["repository"]],
                ["git","-C",str(checkout),"fetch","--tags","origin"],
                ["git","-C",str(checkout),"checkout","--detach",lock["commit"]],
                ["git","-C",str(checkout),"reset","--hard",lock["commit"]],
                ["git","-C",str(checkout),"clean","-ffd"]]
    return [["git","clone",lock["repository"],str(checkout)],
            ["git","-C",str(checkout),"checkout","--detach",lock["commit"]]]

def build_dependency_command(checkout: Path, python_executable: str=sys.executable) -> list[str]:
    return [python_executable,"-m","pip","install","-r",str(checkout/"requirements.txt")]

def build_local_dependency_command(python_executable: str=sys.executable) -> list[str]:
    return [python_executable,"-m","pip","install","-r",str(ROOT/"requirements.txt")]

def bootstrap(checkout: Path, install_deps: bool=True) -> None:
    lock=load_lock(); checkout.parent.mkdir(parents=True, exist_ok=True)
    for cmd in build_bootstrap_commands(checkout,lock): subprocess.run(cmd,check=True)
    head=subprocess.check_output(["git","-C",str(checkout),"rev-parse","HEAD"],text=True).strip()
    if head != lock["commit"]: raise RuntimeError(f"ps3recomp pin mismatch: {head}")
    if install_deps:
        subprocess.run(build_local_dependency_command(),check=True)
        subprocess.run(build_dependency_command(checkout),check=True)
    print(f"ps3recomp ready at {head}")

def main() -> int:
    ap=argparse.ArgumentParser(); ap.add_argument("--checkout",type=Path,default=ROOT/"external"/"ps3recomp"); ap.add_argument("--skip-deps",action="store_true")
    a=ap.parse_args(); bootstrap(a.checkout,not a.skip_deps); return 0
if __name__ == "__main__": raise SystemExit(main())
