from __future__ import annotations
import argparse, json, re, subprocess, sys, tempfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
REG_RE=re.compile(r'ps3_hle_register(?:_ctx)?\(\s*0x([0-9A-Fa-f]{8})u?')
OVERRIDE_RE=re.compile(r'\bNID_[A-Z0-9_]+\s*=\s*0x([0-9A-Fa-f]{8})u?')

def parse_registered_nids(text: str) -> set[int]:
    return {int(x,16) for x in REG_RE.findall(text)}

def parse_override_nids(text: str) -> set[int]:
    return {int(x,16) for x in OVERRIDE_RE.findall(text)}

def uncovered_imports(imports: list[dict], covered: set[int]) -> list[dict]:
    return [x for x in imports if int(str(x['nid']),16) not in covered]

def generate_runtime_nids(ps3recomp: Path) -> tuple[set[int],str]:
    tool=ps3recomp/'tools'/'gen_hle_nids.py'
    if not tool.exists(): raise FileNotFoundError(f'missing {tool}')
    with tempfile.TemporaryDirectory() as td:
        out=Path(td)/'ppu_hle_nids.cpp'
        cp=subprocess.run([sys.executable,str(tool),'--all','--out',str(out)],text=True,capture_output=True)
        if cp.returncode:
            raise RuntimeError(f'gen_hle_nids.py failed ({cp.returncode}):\n{cp.stdout}{cp.stderr}')
        text=out.read_text(encoding='utf-8',errors='replace')
    return parse_registered_nids(text),(cp.stdout+cp.stderr).strip()

def load_imports(path: Path) -> list[dict]:
    obj=json.loads(path.read_text(encoding='utf-8'))
    if isinstance(obj,dict):
        # Accept the local probe format as well as ppu_loader's flat list.
        obj=obj.get('firmware_imports',{}).get('imports',obj.get('imports',[]))
    if not isinstance(obj,list): raise ValueError(f'{path}: expected import list')
    return obj

def collect_port_overrides(paths: list[Path]) -> set[int]:
    out=set()
    for p in paths:
        if not p.exists(): raise FileNotFoundError(p)
        out |= parse_override_nids(p.read_text(encoding='utf-8',errors='replace'))
    return out

def audit(ps3recomp: Path, imports_path: Path, override_paths: list[Path]) -> dict:
    imports=load_imports(imports_path)
    runtime,gen_log=generate_runtime_nids(ps3recomp)
    overrides=collect_port_overrides(override_paths)
    covered=runtime|overrides
    missing=uncovered_imports(imports,covered)
    return {
        'imports':len(imports),
        'runtime_registered_nids':len(runtime),
        'port_override_nids':len(overrides),
        'covered_imports':len(imports)-len(missing),
        'missing':missing,
        'generator_log':gen_log,
    }

def main() -> int:
    ap=argparse.ArgumentParser(description='Fail if a Comet Crash firmware import has no runtime or port HLE registration')
    ap.add_argument('--ps3recomp',type=Path,required=True)
    ap.add_argument('--imports',type=Path,required=True)
    ap.add_argument('--override',type=Path,action='append',default=[])
    ap.add_argument('--json',type=Path)
    a=ap.parse_args()
    overrides=a.override or [ROOT/'port'/'comet_host.cpp',ROOT/'port'/'comet_compat.cpp']
    r=audit(a.ps3recomp,a.imports,overrides)
    if a.json:
        a.json.parent.mkdir(parents=True,exist_ok=True); a.json.write_text(json.dumps(r,indent=2)+'\n')
    print(f'HLE coverage: {r["covered_imports"]}/{r["imports"]} Comet imports covered '
          f'({r["runtime_registered_nids"]} runtime NIDs + {r["port_override_nids"]} port overrides)')
    if r['missing']:
        print('ERROR: unresolved Comet imports would otherwise silently fake success:',file=sys.stderr)
        for x in r['missing']:
            print(f'  {x.get("library",x.get("lib","?"))}: {x["nid"]} stub={x.get("stub","?")}',file=sys.stderr)
        return 1
    return 0

if __name__=='__main__': raise SystemExit(main())
