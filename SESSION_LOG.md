# SESSION LOG

## 2026-09-22 — Advanced checkpoint recovered and first-boot diagnostics hardened

- Received `Comet-Crash-1-Decomp-checkpoint.zip` containing a clean Git repository whose HEAD was original commit `3250838` (`Fix Comet RSX report completion paths`).
- Confirmed the checkpoint's `origin` points to `Stylish-Dark/Comet-Crash-1-Decomp`.
- Verified the current remote no longer exposes the original checkpoint commits, so the uploaded ZIP is a recovered lost advanced history rather than a duplicate of the present GitHub branch.
- Read `docs/STATUS.md` and established that this checkpoint is materially ahead of the temporary reconstructed scaffold: full PPU/SPU lift, HLE coverage gate, Windows host controls, mouse compatibility input, SPURS urgent commands, VFS/RESC/GCM fixes and one-command Windows build/run were already implemented.
- Ran `pytest`: 57 tests + 7 subtests passed on the untouched checkpoint.
- Ran the repository's own CI contract: 57/57 `unittest` tests passed, `compileall` passed, `tools/repo_safety.py` reported `repository safety: OK`.
- Audited the next stated phase (first Windows boot) and found the run path did not preserve console diagnostics to disk.
- Added durable boot logging in `tools/comet_port.py`: combined native stdout/stderr are streamed both to console and a timestamped log; the log header records the command, ELF SHA-256, executable path, title root and non-secret PS3 runtime mappings; exit code is appended.
- Added `--log` override to the `run` command; default path is `logs/boot-YYYYMMDD-HHMMSS.txt`.
- Added granular `[boot-stage]` markers in `port/main.cpp` through process entry, host init, guest VM reservation, ELF load, PARAM.SFO, PPU registration, HLE/sysPrx/fs/LV2 registration, Comet overrides, frame-clock start and PPU entry.
- Added a one-time `[boot-stage] first guest frame presented` marker.
- Added `tests/test_boot_logging.py` covering log streaming, nonzero exit capture, parser support and native stage markers.
- Caught and fixed one fake-Windows syntax-gate issue during implementation (avoided an undeclared test-fixture `InterlockedCompareExchange` by using the first return value of the existing `InterlockedIncrement`).
- Final regression gate after changes: **61/61 tests passed**, `compileall` passed, repository safety passed.

Next action: run the one-command Windows pipeline and use the generated boot log to identify the first actual native blocker.

- Finished the exact-pin VMX compatibility check: pinned ps3recomp `d3ed1a5...` still decodes `vsrb`/`vsrab`, still lacks direct lifter implementations for them, and still emits the exact `/* TODO: ... */;` fallback consumed by `tools/patch_ppu_lift.py`.
- Strengthened `tools/audit_ppu_lift.py` to fail on ps3recomp's separate `unsupported SPR -- no-op` fallback as well as generic `TODO:` holes.
- Added exact ps3recomp checkout verification to `tools/comet_port.py`: analysis/lift/build now reject a non-Git toolkit or a `HEAD` that differs from `config/ps3recomp.lock`.
- Added regression coverage for both failure modes. Local gate: **69/69 tests passed** and `compileall` passed.
- Added first-boot native crash diagnostics in `port/main.cpp`: `SetUnhandledExceptionFilter` records exception code/address, image RVA, crashing thread, and AV read/write/execute target to stderr, which is already mirrored into the durable boot log.
- Extended the fake-Windows compile fixture and boot-log source regression to keep the crash path syntax-checked. Regression gate remains **69/69 tests passed** plus `compileall`.
- Added a non-fatal first-frame watchdog: at 10s and 30s without a presented guest frame it records the last boot stage, flip count, and `g_last_hle_nid`/name breadcrumb from the pinned runtime. This makes deadlocks/spins diagnostically useful without killing the process.
- Regression gate remains **69/69 tests passed** plus `compileall`.
- Hardened `bootstrap_ps3recomp.py` for existing toolchain caches: it now resets `origin` to the locked repository, fetches, checks out the locked commit detached, performs `git reset --hard` to that commit, and `git clean -ffd` before any Comet runtime patches are reapplied. This eliminates state leakage from interrupted previous builds.
- Added bootstrap regression coverage. Local gate: **70/70 tests passed** plus `compileall`.
- Removed the existing-checkout `--skip-deps` shortcut from `scripts/build_and_run.cmd`; step 1 now always runs the full bootstrap so a clone left behind by an interrupted dependency install cannot poison future runs.
- Added launcher regression coverage. Local gate: **71/71 tests passed** plus `compileall`.
- Made analysis deterministic across reruns: `cmd_analyze` now replaces its generated analysis and SPU-extraction directories before invoking ps3recomp, preventing stale extracted images/manifests from surviving into a new lift.
- Added stale-output regression coverage. Local gate: **72/72 tests passed** plus `compileall`.
