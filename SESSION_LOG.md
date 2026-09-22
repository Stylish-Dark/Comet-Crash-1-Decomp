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

- Audited the restored build path against the exact pinned `ps3recomp` commit `d3ed1a5c...`; loader/lifter/SPU CLI contracts and all five Comet runtime patch anchor sites still match the pin.
- Found a real false-green risk in `tools/audit_hle_coverage.py`: any declared `NID_*` constant was previously counted as a port override even when never registered.
- Hardened the HLE gate so symbolic constants count only when passed to `ps3_hle_register[_ctx]` or the port `reg(...)` wrapper; direct literal registrations still count. Added a regression proving declared-only NIDs remain uncovered.
- Regression gate after HLE-audit hardening: **62/62 tests passed** and `compileall` passed.

- Hardened fresh Windows setup: `build_and_run.cmd` now tests `VSCMD_VER` rather than merely seeing a `cl.exe` on PATH, ensuring the Windows SDK/LIB/INCLUDE environment is actually initialized.
- Hardened Ninja discovery: `tools/check_env.py` and `tools/comet_port.py` now accept the executable bundled by the PyPI `ninja` wheel when `ninja.exe` is absent from PATH, and CMake receives that exact executable via `CMAKE_MAKE_PROGRAM`.
- Aligned clang-cl semantics with upstream's recompilation intent: disabled strict-aliasing assumptions and FP contraction explicitly, retained `/bigobj`, and removed the irrelevant clang-cl `/experimental:c11atomics` option.
- Added Ninja discovery regression tests. Regression gate: **64/64 tests passed** and `compileall` passed.

- Added `tools/make_ci_scaffold.py`, which uses ps3recomp's own smoke-header generator plus tiny synthetic PPU/SPU translation units to stage a non-proprietary Windows link fixture.
- Expanded `.github/workflows/tests.yml`: Linux now explicitly installs project requirements, and a Windows job clones the exact ps3recomp lock, applies all five Comet runtime patches, stages synthetic generated code, then compiles/links `CometCrashPC.exe` with clang-cl + Ninja.
- Verified the CI fixture's upstream `make_smoke_elf.py --out/--header` CLI against the exact pinned commit.
- Added CI-scaffold regression tests. Local gate: **66/66 tests passed**, `compileall` passed, and workflow YAML parses.

Next action: run the one-command Windows pipeline and use the generated boot log to identify the first actual native blocker.

- Verified GitHub `main` currently contains the recovered advanced tree plus subsequent hardening commits through `c20cf171` (`Add Windows scaffold build gate`).
- Corrected stale continuity wording/test counts: current local regression gate is **66/66**.
- Recorded an explicit execution-window safety rule: stop substantive work before the session becomes risky, push all completed work, update continuity/next/session state, and only then end the work session.
- No substantive code produced after `c20cf171` was left only in chat; the VMX pin-compatibility check had begun but no new source change was made from it.
