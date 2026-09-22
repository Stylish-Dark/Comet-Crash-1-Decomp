from __future__ import annotations
import argparse, datetime as dt, hashlib, json, os, shutil, subprocess, sys
from pathlib import Path
from sfo import parse_sfo
from ps3_probe import probe_elf
from decrypt_comet_self import decrypt_self
from patch_ppu_lift import patch_file as patch_ppu_file
from audit_spu_lift import audit_file as audit_spu_file
from audit_ppu_lift import audit_path as audit_ppu_path
from patch_ps3recomp_host import patch_file as patch_host_backend
from patch_ps3recomp_spurs import patch_file as patch_spurs_runtime
from patch_ps3recomp_vfs import patch_checkout as patch_vfs_runtime
from patch_ps3recomp_resc import patch_checkout as patch_resc_runtime
from patch_ps3recomp_gcm import patch_checkout as patch_gcm_runtime
from audit_hle_coverage import audit as audit_hle
from audit_analysis import audit_analysis as audit_known_analysis
from check_env import find_ninja
from bootstrap_ps3recomp import load_lock as load_ps3recomp_lock
from boot_triage import classify_signal as classify_boot_signal

ROOT=Path(__file__).resolve().parents[1]
MANIFEST=ROOT/'config'/'comet_crash.json'
DEFAULT_PS3RECOMP=ROOT/'external'/'ps3recomp'

def sha256_file(path: Path) -> str:
    h=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024),b''): h.update(chunk)
    return h.hexdigest()

def load_manifest(): return json.loads(MANIFEST.read_text())
def is_reference_elf_hash(manifest: dict, digest: str) -> bool:
    accepted=set(manifest.get('accepted_elf_sha256',[]))
    accepted.add(manifest.get('reference_elf_sha256',''))
    return digest in accepted
def require_supported_version(manifest: dict, version: str|None, app_version: str|None) -> None:
    if version and manifest.get('version') and version != manifest['version']:
        raise ValueError(f'unsupported title version: {version} (expected {manifest["version"]})')
    if app_version and manifest.get('app_version') and app_version != manifest['app_version']:
        raise ValueError(f'unsupported app version: {app_version} (expected {manifest["app_version"]})')

def require_supported_elf(manifest: dict, digest: str) -> None:
    if not is_reference_elf_hash(manifest,digest):
        raise ValueError(f'unsupported EBOOT reconstruction SHA-256: {digest}')
def validate_supported_elf_file(path: Path) -> str:
    validate_elf_header(path)
    digest=sha256_file(path)
    require_supported_elf(load_manifest(),digest)
    return digest
def find_param_sfo(root: Path) -> Path:
    for p in [root/'PARAM.SFO',root/'PS3_GAME'/'PARAM.SFO']:
        if p.exists(): return p
    raise FileNotFoundError('PARAM.SFO not found')
def validate_elf_header(path: Path):
    b=path.read_bytes()[:20]
    if len(b)<20 or b[:4]!=b'\x7fELF' or b[4]!=2 or b[5]!=2 or int.from_bytes(b[18:20],'big')!=21:
        raise ValueError('ELF must be big-endian ELF64 PPC64')
def validate_inputs(game_root: Path, elf: Path|None=None) -> dict:
    m=load_manifest(); sfo=find_param_sfo(game_root); meta=parse_sfo(sfo)
    got={'title':meta.get('TITLE'),'title_id':meta.get('TITLE_ID'),'version':meta.get('VERSION'),'app_version':meta.get('APP_VER'),'param_sfo_sha256':sha256_file(sfo),'warnings':[]}
    if got['title_id']!=m['title_id']: raise ValueError(f'wrong title id: {got["title_id"]}')
    require_supported_version(m,got['version'],got['app_version'])
    if elf:
        got['elf_sha256']=validate_supported_elf_file(elf); got['elf_size']=elf.stat().st_size
    return got

def run(cmd, cwd=None, env=None):
    print('+',' '.join(map(str,cmd)),flush=True); subprocess.run([str(x) for x in cmd],cwd=cwd,env=env,check=True)

def reset_generated_dir(path: Path) -> None:
    """Replace an ignored/generated directory with an empty one."""
    shutil.rmtree(path,ignore_errors=True)
    path.mkdir(parents=True,exist_ok=True)

BOOT_SIGNAL_MARKERS=(
    '[crash]',
    '[watchdog]',
    '[HLE] UNIMPLEMENTED',
    'unresolved indirect',
    'HOTREAD',
    'unsupported SPU',
    'VM allocation failed',
    'ppu_load_elf failed',
    'D3D12 init FAILED',
)

def update_boot_summary(summary: dict[str,str|None], line: str) -> None:
    text=line.strip()
    if text.startswith('[boot-stage] '):
        summary['last_boot_stage']=text[len('[boot-stage] '):]
    if summary.get('first_signal') is None:
        lower=text.lower()
        for marker in BOOT_SIGNAL_MARKERS:
            if marker.lower() in lower:
                summary['first_signal']=text
                break

def run_logged(cmd, log_path: Path, env=None, metadata: dict|None=None):
    """Run a native boot while mirroring combined stdout/stderr to a durable log."""
    cmd=[str(x) for x in cmd]
    log_path=log_path.resolve(); log_path.parent.mkdir(parents=True,exist_ok=True)
    print('+',' '.join(cmd),flush=True)
    print(f'[boot-log] {log_path}',flush=True)
    with log_path.open('w',encoding='utf-8',newline='\n') as log:
        header={
            'timestamp_utc':dt.datetime.now(dt.timezone.utc).isoformat(),
            'command':cmd,
            **(metadata or {}),
        }
        log.write('# Comet Crash native boot log\n')
        log.write(json.dumps(header,indent=2,sort_keys=True)+'\n\n')
        log.flush()
        summary: dict[str,str|None]={'last_boot_stage':None,'first_signal':None}
        proc=subprocess.Popen(cmd,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
                              text=True,errors='replace',bufsize=1)
        assert proc.stdout is not None
        with proc.stdout:
            for line in proc.stdout:
                print(line,end='',flush=True)
                update_boot_summary(summary,line)
                log.write(line); log.flush()
        rc=proc.wait()
        last=summary['last_boot_stage'] or '<none>'
        signal=summary['first_signal'] or '<none>'
        subsystem,rationale=classify_boot_signal(summary['first_signal'])
        log.write(f'\n# last_boot_stage={last}\n')
        log.write(f'# first_signal={signal}\n')
        log.write(f'# suspected_subsystem={subsystem}\n')
        log.write(f'# triage_rationale={rationale}\n')
        log.write(f'# host_exit_code={rc}\n'); log.flush()
    print(f'[boot-summary] last_stage={last}',flush=True)
    if summary['first_signal']:
        print(f'[boot-summary] first_signal={summary["first_signal"]}',flush=True)
    print(f'[boot-summary] suspected_subsystem={subsystem}',flush=True)
    print(f'[boot-summary] triage_rationale={rationale}',flush=True)
    if rc:
        raise subprocess.CalledProcessError(rc,cmd)
    return rc
def verify_toolkit_checkout(path: Path, expected_commit: str|None=None) -> str:
    if not (path/'tools'/'ppu_loader.py').exists():
        raise FileNotFoundError(f'ps3recomp checkout missing at {path}; run scripts\\bootstrap.cmd')
    if not (path/'.git').exists():
        raise RuntimeError(f'ps3recomp at {path} is not a Git checkout; run scripts\\bootstrap.cmd so the exact locked revision can be verified')
    expected=expected_commit or load_ps3recomp_lock()['commit']
    try:
        head=subprocess.check_output(['git','-C',str(path),'rev-parse','HEAD'],text=True,stderr=subprocess.STDOUT).strip()
    except subprocess.CalledProcessError as e:
        raise RuntimeError(f'could not verify ps3recomp revision at {path}: {e.output.strip()}') from e
    if head != expected:
        raise RuntimeError(f'ps3recomp pin mismatch: checkout={head}, expected={expected}; run scripts\\bootstrap.cmd')
    return head

def require_toolkit(path: Path):
    head=verify_toolkit_checkout(path)
    print(f'ps3recomp pin verified: {head}')

def analysis_commands(elf: Path, ps3recomp: Path, out: Path, spu_dir: Path):
    py=sys.executable
    return [
      [py,ps3recomp/'tools'/'ppu_loader.py',elf,'-o',out],
      [py,ps3recomp/'tools'/'gen_imports.py',elf,'-o',out/'imports_named.json'],
      [py,ps3recomp/'tools'/'extract_spu_images.py',elf,'-o',spu_dir],
    ]
def lift_command(elf: Path, ps3recomp: Path, analysis: Path, output: Path):
    stem=elf.stem
    funcs=analysis/f'{stem}.functions.json'; imports=analysis/f'{stem}.imports.json'
    return [sys.executable,ps3recomp/'tools'/'ppu_lifter.py',elf,'--functions',funcs,'--hle-stubs',imports,'-o',output]
def spu_lift_command(images: Path, ps3recomp: Path, lifted: Path, registry: Path):
    return [sys.executable,ps3recomp/'tools'/'build_spu_workloads.py','--images',images,'--lifted',lifted,'--out',registry,'--register-fn','comet_crash_spu_register_all','--constructor','--title','comet_crash']
def cmake_configure_command(ps3recomp: Path, recomp: Path, spu: Path, registry: Path, build: Path, ninja_program: str|Path|None=None):
    cmd=['cmake','-S',ROOT/'port','-B',build,'-G','Ninja','-DCMAKE_C_COMPILER=clang-cl','-DCMAKE_CXX_COMPILER=clang-cl',f'-DPS3RECOMP_DIR={ps3recomp}',f'-DRECOMP_DIR={recomp}',f'-DSPU_LIFTED_DIR={spu}',f'-DSPU_REGISTRY={registry}']
    if ninja_program: cmd.append(f'-DCMAKE_MAKE_PROGRAM:FILEPATH={ninja_program}')
    return cmd

def runtime_environment(title_root: Path, param_sfo: Path) -> dict[str,str]:
    m=load_manifest()
    title_root=title_root.resolve(); usrdir=(title_root/'USRDIR').resolve()
    return {
        'PS3_VFS_ROOT': str(title_root),
        'PS3_HDD_GAME_ROOT': str(title_root),
        'PS3_APP_HOME_ROOT': str(usrdir),
        'PS3_WORKING_ROOT': str(usrdir),
        'PS3_TITLE_ID': m['title_id'],
        'PS3_PARAM_SFO': str(param_sfo.resolve()),
        'PS3_TITLE': m['title'],
    }

def cmd_decrypt(a): print(json.dumps(decrypt_self(a.input,a.output),indent=2))
def cmd_validate(a): print(json.dumps(validate_inputs(a.game,a.elf),indent=2))
def cmd_probe(a):
    r=probe_elf(a.elf); a.output.parent.mkdir(parents=True,exist_ok=True); a.output.write_text(json.dumps(r,indent=2)+'\n'); print(json.dumps(r,indent=2))
def cmd_analyze(a):
    require_toolkit(a.ps3recomp); validate_inputs(a.game,a.elf)
    reset_generated_dir(a.output); reset_generated_dir(a.spu)
    for c in analysis_commands(a.elf,a.ps3recomp,a.output,a.spu): run(c)
    stem=a.elf.stem
    baseline=audit_known_analysis(a.output/f'{stem}.loader.json',a.output/f'{stem}.imports.json',a.spu)
    print(f'Analysis baseline gate: PPU={baseline["ppu"]["function_count"]} funcs, imports={baseline["imports"]}/{baseline["libraries"]} libs, SPUs={baseline["spu_images"]}')
    if not baseline['ok']:
        raise RuntimeError('analysis drift from known Comet Crash baseline: '+'; '.join(baseline['errors']))
    local=probe_elf(a.elf); (a.output/'local_probe.json').write_text(json.dumps(local,indent=2)+'\n')
def cmd_lift(a):
    require_toolkit(a.ps3recomp); validate_supported_elf_file(a.elf)
    if a.clean:
        shutil.rmtree(a.output,ignore_errors=True); shutil.rmtree(a.spu_output,ignore_errors=True)
        if a.spu_registry.exists(): a.spu_registry.unlink()
    a.output.mkdir(parents=True,exist_ok=True)
    run(lift_command(a.elf,a.ps3recomp,a.analysis,a.output))
    ppu_patch={'vsrab':0,'vsrb':0}
    for cpp in sorted(a.output.glob('ppu_recomp_*.cpp')):
        st=patch_ppu_file(cpp)
        for k,v in st.items(): ppu_patch[k]+=v
    print(f'Comet Crash PPU compatibility patch: {ppu_patch}')
    ppu_report=audit_ppu_path(a.output)
    print(f'PPU audit: files={ppu_report["source_files"]}, unsupported={ppu_report["unsupported_total"]}')
    if ppu_report['unsupported_total']:
        sample=', '.join(str(x['instruction']) for x in ppu_report['unsupported'][:5])
        raise RuntimeError(f'PPU lift still contains {ppu_report["unsupported_total"]} unsupported TODO instruction(s): {sample}')
    images=list(a.spu_images.glob('*.elf'))
    if not images: raise FileNotFoundError(f'no extracted SPU ELFs in {a.spu_images}; run analyze first')
    a.spu_output.mkdir(parents=True,exist_ok=True); a.spu_registry.parent.mkdir(parents=True,exist_ok=True)
    run(spu_lift_command(a.spu_images,a.ps3recomp,a.spu_output,a.spu_registry))
    for img in sorted(images):
        b=img.read_bytes()
        if len(b)<0x1c: raise ValueError(f'truncated SPU ELF: {img}')
        entry=int.from_bytes(b[0x18:0x1c],'big')
        src=a.spu_output/img.stem/'spu_recomp.c'
        report=audit_spu_file(src,entry)
        print(f'SPU audit {img.name}: reachable={report["reachable_functions"]}/{report["functions_total"]}, unsupported_reachable={report["unsupported_reachable"]}')
        if report['unsupported_reachable']:
            raise RuntimeError(f'{img.name}: {report["unsupported_reachable"]} reachable unsupported SPU instructions')
def cmd_build(a):
    require_toolkit(a.ps3recomp)
    if a.clean: shutil.rmtree(a.build,ignore_errors=True)
    a.build.mkdir(parents=True,exist_ok=True)
    backend=a.ps3recomp/'libs'/'video'/'rsx_d3d12_backend.c'
    changed=patch_host_backend(backend)
    print(f'Comet host D3D12 patch: {"applied" if changed else "already present"}')
    spurs=a.ps3recomp/'libs'/'spurs'/'cellSpurs.c'
    changed=patch_spurs_runtime(spurs)
    print(f'Comet SPURS urgent-command patch: {"applied" if changed else "already present"}')
    vfs_changes=patch_vfs_runtime(a.ps3recomp)
    print('Comet HDD-title VFS patch: '+', '.join(f'{k}={"applied" if v else "already present"}' for k,v in vfs_changes.items()))
    resc_changed=patch_resc_runtime(a.ps3recomp)
    print(f'Comet RESC guest-memory patch: {"applied" if resc_changed else "already present"}')
    gcm_changed=patch_gcm_runtime(a.ps3recomp)
    print(f'Comet GCM report patch: {"applied" if gcm_changed else "already present"}')
    if not a.imports.exists():
        raise FileNotFoundError(f'Comet import manifest missing at {a.imports}; run analyze first')
    hle=audit_hle(a.ps3recomp,a.imports,[ROOT/'port'/'comet_host.cpp',ROOT/'port'/'comet_compat.cpp'])
    print(f'HLE coverage gate: {hle["covered_imports"]}/{hle["imports"]} Comet imports covered')
    if hle['missing']:
        details=', '.join(f'{x.get("library",x.get("lib","?"))}:{x["nid"]}' for x in hle['missing'])
        raise RuntimeError(f'unresolved Comet HLE imports: {details}')
    ninja=find_ninja()
    if not ninja: raise FileNotFoundError('Ninja not found on PATH or in the Python ninja package')
    print(f'Ninja: {ninja}')
    run(cmake_configure_command(a.ps3recomp,a.recomp,a.spu,a.spu_registry,a.build,ninja)); run(['cmake','--build',a.build])
def cmd_run(a):
    validate_inputs(a.game,a.elf)
    exe=a.exe or a.build/'CometCrashPC.exe'
    sfo=find_param_sfo(a.game); title_root=sfo.parent
    runtime_env=runtime_environment(title_root,sfo)
    env=os.environ.copy(); env.update(runtime_env)
    if a.log:
        log_path=a.log
    else:
        stamp=dt.datetime.now().strftime('%Y%m%d-%H%M%S')
        log_path=ROOT/'logs'/f'boot-{stamp}.txt'
    metadata={
        'exe':str(Path(exe).resolve()),
        'elf':str(a.elf.resolve()),
        'elf_sha256':sha256_file(a.elf),
        'title_root':str(title_root.resolve()),
        'runtime_environment':runtime_env,
    }
    run_logged([exe,a.elf],log_path,env=env,metadata=metadata)
def parser():
    p=argparse.ArgumentParser(description='Comet Crash native-port pipeline'); s=p.add_subparsers(dest='cmd',required=True)
    q=s.add_parser('decrypt'); q.add_argument('input',type=Path); q.add_argument('-o','--output',type=Path,default=ROOT/'work'/'EBOOT.ELF'); q.set_defaults(func=cmd_decrypt)
    q=s.add_parser('validate'); q.add_argument('game',type=Path); q.add_argument('--elf',type=Path); q.set_defaults(func=cmd_validate)
    q=s.add_parser('probe'); q.add_argument('elf',type=Path); q.add_argument('-o','--output',type=Path,default=ROOT/'out'/'local_probe.json'); q.set_defaults(func=cmd_probe)
    q=s.add_parser('analyze'); q.add_argument('game',type=Path); q.add_argument('elf',type=Path); q.add_argument('--ps3recomp',type=Path,default=DEFAULT_PS3RECOMP); q.add_argument('-o','--output',type=Path,default=ROOT/'out'); q.add_argument('--spu',type=Path,default=ROOT/'work'/'spu'); q.set_defaults(func=cmd_analyze)
    q=s.add_parser('lift'); q.add_argument('elf',type=Path); q.add_argument('--ps3recomp',type=Path,default=DEFAULT_PS3RECOMP); q.add_argument('--analysis',type=Path,default=ROOT/'out'); q.add_argument('-o','--output',type=Path,default=ROOT/'generated'/'recompiled'); q.add_argument('--spu-images',type=Path,default=ROOT/'work'/'spu'); q.add_argument('--spu-output',type=Path,default=ROOT/'generated'/'spu'); q.add_argument('--spu-registry',type=Path,default=ROOT/'generated'/'spu_workloads.c'); q.add_argument('--clean',action='store_true'); q.set_defaults(func=cmd_lift)
    q=s.add_parser('build'); q.add_argument('--ps3recomp',type=Path,default=DEFAULT_PS3RECOMP); q.add_argument('--recomp',type=Path,default=ROOT/'generated'/'recompiled'); q.add_argument('--spu',type=Path,default=ROOT/'generated'/'spu'); q.add_argument('--spu-registry',type=Path,default=ROOT/'generated'/'spu_workloads.c'); q.add_argument('--imports',type=Path,default=ROOT/'out'/'EBOOT.imports.json'); q.add_argument('--build',type=Path,default=ROOT/'build'); q.add_argument('--clean',action='store_true',help='remove the CMake build directory before configuring'); q.set_defaults(func=cmd_build)
    q=s.add_parser('run'); q.add_argument('game',type=Path); q.add_argument('elf',type=Path); q.add_argument('--build',type=Path,default=ROOT/'build'); q.add_argument('--exe',type=Path); q.add_argument('--log',type=Path,help='boot log path (default: logs/boot-YYYYMMDD-HHMMSS.txt)'); q.set_defaults(func=cmd_run)
    return p
def main():
    a=parser().parse_args(); a.func(a)
if __name__=='__main__': main()
