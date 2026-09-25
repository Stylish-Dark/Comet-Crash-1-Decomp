from __future__ import annotations
import argparse, datetime as dt, hashlib, json, os, shutil, subprocess, sys, threading
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
from patch_ps3recomp_thread_exit import patch_file as patch_thread_exit_runtime
from patch_ps3recomp_parse_watch import patch_file as patch_parse_watch_runtime
from audit_hle_coverage import audit as audit_hle
from audit_analysis import audit_analysis as audit_known_analysis
from check_env import find_ninja
from bootstrap_ps3recomp import load_lock as load_ps3recomp_lock
from boot_triage import (
    MEMALIGN_MARKERS,
    extract_malloc_source,
    extract_parse_corruption,
    extract_parse_write,
    classify_signal as classify_boot_signal,
    derive_outcome as derive_boot_outcome,
)

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

def git_state(path: Path) -> dict[str,object]:
    """Return an exact tracked-source snapshot without mutating the checkout."""
    if not (path/'.git').exists():
        return {'commit':None,'dirty':None,'tracked_diff_sha256':None}
    commit=subprocess.check_output(
        ['git','-C',str(path),'rev-parse','HEAD'],text=True,stderr=subprocess.STDOUT
    ).strip()
    diff=subprocess.check_output(
        ['git','-C',str(path),'diff','--binary','HEAD','--','.'],
        stderr=subprocess.STDOUT,
    )
    return {
        'commit':commit,
        'dirty':bool(diff),
        'tracked_diff_sha256':hashlib.sha256(diff).hexdigest(),
    }

def make_build_provenance(*, project_root: Path, ps3recomp_commit: str,
                          hle: dict[str,object], recomp: Path, spu: Path,
                          executable: Path) -> dict[str,object]:
    executable=executable.resolve()
    if not executable.is_file():
        raise FileNotFoundError(f'native executable missing after build: {executable}')
    return {
        'schema_version':2,
        'built_at_utc':dt.datetime.now(dt.timezone.utc).isoformat(),
        'port_git':git_state(project_root),
        'ps3recomp_commit':ps3recomp_commit,
        'hle_coverage':{
            'imports':int(hle['imports']),
            'covered_imports':int(hle['covered_imports']),
            'missing':len(hle.get('missing',[])),
        },
        'generated_ppu_chunks':len(list(recomp.glob('ppu_recomp_*.cpp'))),
        'generated_spu_units':len(list(spu.glob('*/spu_recomp.c'))),
        'native_executable':{
            'name':executable.name,
            'size':executable.stat().st_size,
            'sha256':sha256_file(executable),
        },
    }

def write_build_provenance(build: Path, provenance: dict[str,object]) -> Path:
    path=build/'build_provenance.json'
    path.write_text(json.dumps(provenance,indent=2,sort_keys=True)+'\n',encoding='utf-8',newline='\n')
    return path

def verify_build_provenance(provenance_path: Path, executable: Path,
                            project_root: Path, expected_ps3recomp_commit: str) -> dict[str,object]:
    """Fail if a native executable is not exactly bound to the current source/toolchain state."""
    if not provenance_path.is_file():
        raise FileNotFoundError(f'build provenance missing: {provenance_path}; rebuild before running')
    executable=executable.resolve()
    if not executable.is_file():
        raise FileNotFoundError(f'native executable missing: {executable}')
    provenance=json.loads(provenance_path.read_text(encoding='utf-8'))
    errors=[]
    if provenance.get('schema_version') != 2:
        errors.append(f'unsupported provenance schema {provenance.get("schema_version")!r}')
    artifact=provenance.get('native_executable') or {}
    actual_size=executable.stat().st_size
    actual_sha=sha256_file(executable)
    if artifact.get('size') != actual_size:
        errors.append(f'executable size mismatch: built={artifact.get("size")}, actual={actual_size}')
    if artifact.get('sha256') != actual_sha:
        errors.append(f'executable SHA-256 mismatch: built={artifact.get("sha256")}, actual={actual_sha}')
    if provenance.get('ps3recomp_commit') != expected_ps3recomp_commit:
        errors.append(
            f'ps3recomp provenance mismatch: built={provenance.get("ps3recomp_commit")}, '
            f'expected={expected_ps3recomp_commit}'
        )
    hle=provenance.get('hle_coverage') or {}
    if hle.get('imports') != 171 or hle.get('covered_imports') != 171 or hle.get('missing') != 0:
        errors.append(f'HLE provenance is not complete for the 171-import Comet baseline: {hle}')
    built_git=provenance.get('port_git') or {}
    current_git=git_state(project_root)
    if built_git.get('commit') != current_git.get('commit'):
        errors.append(
            f'port commit mismatch: built={built_git.get("commit")}, current={current_git.get("commit")}'
        )
    if built_git.get('tracked_diff_sha256') != current_git.get('tracked_diff_sha256'):
        errors.append('tracked source diff changed since native build')
    if errors:
        raise RuntimeError('native build provenance check failed: '+'; '.join(errors))
    return provenance

BOOT_SIGNAL_MARKERS=(
    '[crash]',
    '[watchdog]',
    '[HLE] UNIMPLEMENTED',
    'unresolved indirect',
    'HOTREAD',
    'unsupported SPU',
    'VM allocation failed',
    'ppu_load_elf failed',
    '[D3D12] ERROR:',
    'D3D12 init FAILED',
    '[COMET-MEMALIGN-MALLOC-LOW]',
    '[COMET-MEMALIGN-CORE-LOW]',
    '[COMET-MEMALIGN-WRAPPER-LOW]',
    '[COMET-MALLOC-LOW]',
    '[COMET-PARSE-WRITE]',
    '[COMET-PARSE-WRITE-HLE]',
    '[COMET-PARSE-REG-CLOBBER]',
    '[COMET-PARSE-SP-CHANGE]',
    '[COMET-PARSE-SLOT-CHANGE]',
    '[COMET-ALLOC-CORRUPTION]',
)

def update_boot_summary(summary: dict[str,object], line: str) -> None:
    text=line.strip()
    if text.startswith('[boot-stage] '):
        stage=text[len('[boot-stage] '):]
        summary['last_boot_stage']=stage
        if stage=='first guest frame presented':
            summary['first_frame_presented']=True
    lower=text.lower()
    write_hit=extract_parse_write(text)
    if write_hit is not None and summary.get('parse_write_kind') is None:
        summary['parse_write_kind'],summary['parse_write_function']=write_hit
        summary['parse_write_signal']=text
    parse_hit=extract_parse_corruption(text)
    if parse_hit is not None and summary.get('parse_corruption_kind') is None:
        summary['parse_corruption_kind'],summary['parse_corruption_site']=parse_hit
        summary['parse_corruption_signal']=text
    source=extract_malloc_source(text)
    if source is not None and summary.get('malloc_source') is None:
        summary['malloc_source']=source
        summary['malloc_signal']=text
    hits=summary.setdefault('memalign_hits',{})
    if isinstance(hits,dict):
        for origin, marker in MEMALIGN_MARKERS:
            if marker.lower() in lower and origin not in hits:
                hits[origin]=text
    detected=False
    for marker in BOOT_SIGNAL_MARKERS:
        if marker.lower() in lower:
            detected=True
            break
    if not detected:
        return
    if summary.get('first_signal') is None:
        summary['first_signal']=text
    subsystem,_=classify_boot_signal(text)
    if subsystem!='unknown' and summary.get('first_specific_signal') is None:
        summary['first_specific_signal']=text

def _stop_process(proc: subprocess.Popen) -> int:
    """Terminate a stuck native process, escalating to kill after a short grace period."""
    if proc.poll() is not None:
        return int(proc.returncode or 0)
    proc.terminate()
    try:
        return int(proc.wait(timeout=5))
    except subprocess.TimeoutExpired:
        proc.kill()
        return int(proc.wait())

def run_logged(cmd, log_path: Path, env=None, metadata: dict|None=None, timeout_seconds: float|None=None):
    """Run a native boot while mirroring combined stdout/stderr to a durable log."""
    if timeout_seconds is not None and timeout_seconds <= 0:
        raise ValueError('timeout_seconds must be > 0')
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
        if timeout_seconds is not None:
            header['timeout_seconds']=timeout_seconds
        log.write('# Comet Crash native boot log\n')
        log.write(json.dumps(header,indent=2,sort_keys=True)+'\n\n')
        log.flush()
        summary: dict[str,object]={
            'last_boot_stage':None,
            'first_signal':None,
            'first_specific_signal':None,
            'first_frame_presented':False,
            'memalign_hits':{},
            'malloc_source':None,
            'malloc_signal':None,
            'parse_corruption_kind':None,
            'parse_corruption_site':None,
            'parse_corruption_signal':None,
            'parse_write_kind':None,
            'parse_write_function':None,
            'parse_write_signal':None,
        }
        proc=subprocess.Popen(cmd,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
                              text=True,errors='replace',bufsize=1)
        assert proc.stdout is not None
        timed_out=False
        interrupted: str|None=None
        timer: threading.Timer|None=None

        if timeout_seconds is not None:
            def timeout_proc() -> None:
                nonlocal timed_out
                if proc.poll() is None:
                    timed_out=True
                    print(f'[boot-timeout] terminating native process after {timeout_seconds:g}s',flush=True)
                    try:
                        _stop_process(proc)
                    except OSError:
                        pass
            timer=threading.Timer(timeout_seconds,timeout_proc)
            timer.daemon=True
            timer.start()

        try:
            with proc.stdout:
                for line in proc.stdout:
                    print(line,end='',flush=True)
                    update_boot_summary(summary,line)
                    log.write(line); log.flush()
            rc=proc.wait()
        except KeyboardInterrupt:
            interrupted='keyboard'
            print('[boot-interrupt] Ctrl+C received; terminating native process',flush=True)
            rc=_stop_process(proc)
        finally:
            if timer is not None:
                timer.cancel()

        last=summary['last_boot_stage'] or '<none>'
        signal=summary['first_signal'] or '<none>'
        triage_signal=summary['first_specific_signal'] or summary['first_signal']
        triage_display=triage_signal or '<none>'
        subsystem,rationale=classify_boot_signal(triage_signal)
        memalign_origin=None
        memalign_signal=None
        hits=summary.get('memalign_hits')
        if isinstance(hits,dict):
            for origin,_ in MEMALIGN_MARKERS:
                if origin in hits:
                    memalign_origin=origin
                    memalign_signal=hits[origin]
                    break
        first_frame=bool(summary['first_frame_presented'])
        outcome=derive_boot_outcome(
            first_frame_presented=first_frame,
            host_exit_code=rc,
            timed_out=timed_out,
            interrupted=interrupted,
            first_signal=summary['first_signal'] if isinstance(summary['first_signal'],str) else None,
        )
        log.write(f'\n# last_boot_stage={last}\n')
        log.write(f'# first_signal={signal}\n')
        log.write(f'# triage_signal={triage_display}\n')
        log.write(f'# suspected_subsystem={subsystem}\n')
        log.write(f'# triage_rationale={rationale}\n')
        log.write(f'# memalign_origin={memalign_origin or "<none>"}\n')
        log.write(f'# memalign_signal={memalign_signal or "<none>"}\n')
        log.write(f'# malloc_source={summary.get("malloc_source") or "<none>"}\n')
        log.write(f'# malloc_signal={summary.get("malloc_signal") or "<none>"}\n')
        log.write(f'# parse_corruption_kind={summary.get("parse_corruption_kind") or "<none>"}\n')
        log.write(f'# parse_corruption_site={summary.get("parse_corruption_site") or "<none>"}\n')
        log.write(f'# parse_corruption_signal={summary.get("parse_corruption_signal") or "<none>"}\n')
        log.write(f'# parse_write_kind={summary.get("parse_write_kind") or "<none>"}\n')
        log.write(f'# parse_write_function={summary.get("parse_write_function") or "<none>"}\n')
        log.write(f'# parse_write_signal={summary.get("parse_write_signal") or "<none>"}\n')
        log.write(f'# first_frame_presented={"true" if first_frame else "false"}\n')
        log.write(f'# interrupted={interrupted or "<none>"}\n')
        log.write(f'# timed_out={"true" if timed_out else "false"}\n')
        log.write(f'# boot_outcome={outcome}\n')
        log.write(f'# host_exit_code={rc}\n'); log.flush()
    sidecar=log_path.with_suffix('.summary.json')
    sidecar.write_text(json.dumps({
        'summary_schema_version':1,
        'log_path':str(log_path),
        'boot_log_sha256':sha256_file(log_path),
        'elf_sha256':(metadata or {}).get('elf_sha256'),
        'last_boot_stage':None if last=='<none>' else last,
        'first_signal':summary['first_signal'],
        'triage_signal':triage_signal,
        'suspected_subsystem':subsystem,
        'triage_rationale':rationale,
        'memalign_origin':memalign_origin,
        'memalign_signal':memalign_signal,
        'malloc_source':summary.get('malloc_source'),
        'malloc_signal':summary.get('malloc_signal'),
        'parse_corruption_kind':summary.get('parse_corruption_kind'),
        'parse_corruption_site':summary.get('parse_corruption_site'),
        'parse_corruption_signal':summary.get('parse_corruption_signal'),
        'parse_write_kind':summary.get('parse_write_kind'),
        'parse_write_function':summary.get('parse_write_function'),
        'parse_write_signal':summary.get('parse_write_signal'),
        'first_frame_presented':first_frame,
        'interrupted':interrupted,
        'timed_out':timed_out,
        'boot_outcome':outcome,
        'host_exit_code':rc,
        'build_provenance':(metadata or {}).get('build_provenance'),
        'provenance_verification':(metadata or {}).get('provenance_verification'),
    },indent=2,sort_keys=True)+'\n',encoding='utf-8',newline='\n')
    print(f'[boot-summary-json] {sidecar}',flush=True)
    print(f'[boot-summary] last_stage={last}',flush=True)
    if summary['first_signal']:
        print(f'[boot-summary] first_signal={summary["first_signal"]}',flush=True)
    if triage_signal:
        print(f'[boot-summary] triage_signal={triage_signal}',flush=True)
    print(f'[boot-summary] suspected_subsystem={subsystem}',flush=True)
    if memalign_origin:
        print(f'[boot-summary] memalign_origin={memalign_origin}',flush=True)
        print(f'[boot-summary] memalign_signal={memalign_signal}',flush=True)
    if summary.get('malloc_source'):
        print(f'[boot-summary] malloc_source={summary["malloc_source"]}',flush=True)
        print(f'[boot-summary] malloc_signal={summary["malloc_signal"]}',flush=True)
    if summary.get('parse_write_kind'):
        print(f'[boot-summary] parse_write_kind={summary["parse_write_kind"]}',flush=True)
        print(f'[boot-summary] parse_write_function={summary["parse_write_function"]}',flush=True)
        print(f'[boot-summary] parse_write_signal={summary["parse_write_signal"]}',flush=True)
    if summary.get('parse_corruption_kind'):
        print(f'[boot-summary] parse_corruption_kind={summary["parse_corruption_kind"]}',flush=True)
        print(f'[boot-summary] parse_corruption_site={summary["parse_corruption_site"]}',flush=True)
        print(f'[boot-summary] parse_corruption_signal={summary["parse_corruption_signal"]}',flush=True)
    print(f'[boot-summary] boot_outcome={outcome}',flush=True)
    print(f'[boot-summary] triage_rationale={rationale}',flush=True)
    if interrupted:
        raise KeyboardInterrupt
    if timed_out:
        raise subprocess.TimeoutExpired(cmd,timeout_seconds)
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

def require_toolkit(path: Path) -> str:
    head=verify_toolkit_checkout(path)
    print(f'ps3recomp pin verified: {head}')
    return head

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
    print(f'PPU audit: files={ppu_report["source_files"]}, unsupported={ppu_report["unsupported_total"]}, raw-word={ppu_report["raw_word_total"]} sha256={ppu_report["raw_word_sha256"]}')
    if ppu_report['unsupported_total']:
        sample=', '.join(str(x['instruction']) for x in ppu_report['unsupported'][:5])
        raise RuntimeError(f'PPU lift still contains {ppu_report["unsupported_total"]} unsupported TODO instruction(s): {sample}')
    known=json.loads((ROOT/'docs'/'reference'/'known-analysis.json').read_text(encoding='utf-8')).get('ppu_lift',{})
    expected_raw_count=int(known.get('raw_word_todo_count',-1))
    expected_raw_sha=str(known.get('raw_word_todo_sha256','')).lower()
    if ppu_report['raw_word_total'] != expected_raw_count or ppu_report['raw_word_sha256'].lower() != expected_raw_sha:
        raise RuntimeError('PPU raw-word TODO baseline mismatch: '
                           f'got count={ppu_report["raw_word_total"]} sha256={ppu_report["raw_word_sha256"]}, '
                           f'expected count={expected_raw_count} sha256={expected_raw_sha}')
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
    ps3recomp_commit=require_toolkit(a.ps3recomp)
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
    thread_exit_changed=patch_thread_exit_runtime(a.ps3recomp)
    print(f'Comet Windows PPU thread-exit patch: {"applied" if thread_exit_changed else "already present"}')
    parse_watch_changed=patch_parse_watch_runtime(a.ps3recomp)
    print(f'Comet parse-slot runtime watch: {"applied" if parse_watch_changed else "already present"}')
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
    executable=a.build/'CometCrashPC.exe'
    provenance=make_build_provenance(
        project_root=ROOT,
        ps3recomp_commit=ps3recomp_commit,
        hle=hle,
        recomp=a.recomp,
        spu=a.spu,
        executable=executable,
    )
    provenance_path=write_build_provenance(a.build,provenance)
    print(f'Build provenance: {provenance_path}')
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
    provenance_path=a.build/'build_provenance.json'
    expected_ps3recomp=load_ps3recomp_lock()['commit']
    try:
        build_provenance=verify_build_provenance(
            provenance_path,
            Path(exe),
            ROOT,
            expected_ps3recomp,
        )
        provenance_verification={'status':'verified'}
    except (FileNotFoundError,RuntimeError,ValueError,json.JSONDecodeError) as e:
        if not a.allow_unprovenanced:
            raise
        print(f'[provenance-warning] {e}',flush=True)
        provenance_verification={
            'status':'overridden',
            'error':str(e),
            'current_port_git':git_state(ROOT),
            'actual_executable':{
                'path':str(Path(exe).resolve()),
                'exists':Path(exe).is_file(),
                'size':Path(exe).stat().st_size if Path(exe).is_file() else None,
                'sha256':sha256_file(Path(exe)) if Path(exe).is_file() else None,
            },
        }
        build_provenance=(
            json.loads(provenance_path.read_text(encoding='utf-8'))
            if provenance_path.is_file()
            else {'status':'missing','path':str(provenance_path.resolve())}
        )
    metadata={
        'exe':str(Path(exe).resolve()),
        'elf':str(a.elf.resolve()),
        'elf_sha256':sha256_file(a.elf),
        'title_root':str(title_root.resolve()),
        'runtime_environment':runtime_env,
        'build_provenance':build_provenance,
        'provenance_verification':provenance_verification,
    }
    run_logged([exe,a.elf],log_path,env=env,metadata=metadata,timeout_seconds=a.timeout)
def parser():
    p=argparse.ArgumentParser(description='Comet Crash native-port pipeline'); s=p.add_subparsers(dest='cmd',required=True)
    q=s.add_parser('decrypt'); q.add_argument('input',type=Path); q.add_argument('-o','--output',type=Path,default=ROOT/'work'/'EBOOT.ELF'); q.set_defaults(func=cmd_decrypt)
    q=s.add_parser('validate'); q.add_argument('game',type=Path); q.add_argument('--elf',type=Path); q.set_defaults(func=cmd_validate)
    q=s.add_parser('probe'); q.add_argument('elf',type=Path); q.add_argument('-o','--output',type=Path,default=ROOT/'out'/'local_probe.json'); q.set_defaults(func=cmd_probe)
    q=s.add_parser('analyze'); q.add_argument('game',type=Path); q.add_argument('elf',type=Path); q.add_argument('--ps3recomp',type=Path,default=DEFAULT_PS3RECOMP); q.add_argument('-o','--output',type=Path,default=ROOT/'out'); q.add_argument('--spu',type=Path,default=ROOT/'work'/'spu'); q.set_defaults(func=cmd_analyze)
    q=s.add_parser('lift'); q.add_argument('elf',type=Path); q.add_argument('--ps3recomp',type=Path,default=DEFAULT_PS3RECOMP); q.add_argument('--analysis',type=Path,default=ROOT/'out'); q.add_argument('-o','--output',type=Path,default=ROOT/'generated'/'recompiled'); q.add_argument('--spu-images',type=Path,default=ROOT/'work'/'spu'); q.add_argument('--spu-output',type=Path,default=ROOT/'generated'/'spu'); q.add_argument('--spu-registry',type=Path,default=ROOT/'generated'/'spu_workloads.c'); q.add_argument('--clean',action='store_true'); q.set_defaults(func=cmd_lift)
    q=s.add_parser('build'); q.add_argument('--ps3recomp',type=Path,default=DEFAULT_PS3RECOMP); q.add_argument('--recomp',type=Path,default=ROOT/'generated'/'recompiled'); q.add_argument('--spu',type=Path,default=ROOT/'generated'/'spu'); q.add_argument('--spu-registry',type=Path,default=ROOT/'generated'/'spu_workloads.c'); q.add_argument('--imports',type=Path,default=ROOT/'out'/'EBOOT.imports.json'); q.add_argument('--build',type=Path,default=ROOT/'build'); q.add_argument('--clean',action='store_true',help='remove the CMake build directory before configuring'); q.set_defaults(func=cmd_build)
    q=s.add_parser('run'); q.add_argument('game',type=Path); q.add_argument('elf',type=Path); q.add_argument('--build',type=Path,default=ROOT/'build'); q.add_argument('--exe',type=Path); q.add_argument('--log',type=Path,help='boot log path (default: logs/boot-YYYYMMDD-HHMMSS.txt)'); q.add_argument('--timeout',type=float,help='optional native-run timeout in seconds; the default is unlimited'); q.add_argument('--allow-unprovenanced',action='store_true',help='diagnostic override: run even when executable/build provenance cannot be verified'); q.set_defaults(func=cmd_run)
    return p
def main():
    a=parser().parse_args(); a.func(a)
if __name__=='__main__': main()
