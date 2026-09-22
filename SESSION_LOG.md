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

## 2026-09-22 — Exact-pin PPU completeness and real Windows compile gate

- Finished the exact-pin VMX compatibility check against `ps3recomp` commit `d3ed1a5c946a9c5370b51631e13371a1adf70396`: the disassembler still recognizes `vsrb` and `vsrab`, the lifter still has no handlers for either, and unsupported operations still fall through to the exact `/* TODO: ... */;` form consumed by `tools/patch_ppu_lift.py`.
- Added `tools/audit_ppu_lift.py` and wired it immediately after the Comet PPU compatibility rewrite. A fresh lift now fails if no expected PPU chunks exist or if any generated `/* TODO: ... */;` remains.
- Validated through PR #1 instead of pushing unproven code directly to `main`.
- The first PR run caught an escaped-regex transfer bug in the new audit file and a CI-only C-vs-C++ linkage mismatch in the synthetic PPU fixture; both were fixed on the branch before merge.
- The initial Windows run was already informative: pinned checkout, all five runtime patches and fixture staging passed, and compilation reached 145/154 objects before the synthetic linkage mismatch stopped it.
- Corrected the fixture to match the exact generated `ppu_recomp.h` linkage contract.
- PR run `35679740111` then passed completely: **71/71 unit tests**, `compileall`, repository safety, and the Windows scaffold compile/link job all succeeded. The Windows runner compiled and linked `CometCrashPC.exe` against the exact pinned runtime with all Comet patches applied.
- Squash-merged PR #1 to `main` as `076ccac39ded89c96743ec6e2988ed9a28f3c64b` (`Gate fresh PPU lifts on unsupported instructions`).

Next action remains the first real Windows build/run against the user's extracted NPEB00142 v1.00 files; static and synthetic pre-boot gates are now materially stronger.

## 2026-09-22 — Real pinned PPU lifter proven on Windows

- Replaced the hand-written synthetic PPU CI source with a four-byte non-proprietary big-endian PPC fixture containing one `blr` instruction at guest address `0x10000`.
- Windows CI now invokes the exact pinned `ppu_lifter.py` on that fixture, producing genuine `ppu_recomp.h` and split `ppu_recomp_*.cpp` output.
- The first real-lifter run exposed a Windows encoding failure: ps3recomp-generated C++ contained CP-1252 comment bytes while the new PPU audit assumed UTF-8.
- Hardened both `patch_ppu_lift.py` and `audit_ppu_lift.py` to treat generated source byte-safely. Patching now uses a reversible Latin-1 byte mapping, preserving all original bytes except the intentional ASCII VMX replacement; auditing uses the same byte-stable approach.
- Added regression coverage for CP-1252/non-UTF-8 generated-source bytes and made CI mirror the real Comet order: pinned lifter -> compatibility patch -> completeness audit -> compile/link.
- PR #2 run `35680385634` passed completely: **74/74 unit tests**, `compileall`, repository safety, real pinned PPU lift, compatibility patch, PPU completeness audit, and Windows clang-cl compile/link all succeeded.
- Squash-merged PR #2 to `main` as `a565de86fb5e4370bd51d69f82b4b2ff3e316098` (`Exercise real pinned PPU lifter in Windows CI`).

Remaining synthetic generated-code coverage in the Windows gate is the SPU side; the real Comet first native boot still remains the decisive runtime boundary.

## 2026-09-22 — Reconciled later pin/no-op hardening without regressions

- A later `main` commit (`b880c3e`) added two useful protections: exact ps3recomp checkout verification and rejection of the lifter's `unsupported SPR -- no-op` fallback.
- The same commit unintentionally reverted previously proven hardening/continuity state, including byte-safe Windows PPU source auditing, Ninja-wheel handoff, and the 74-test/Windows-CI continuity record.
- Found a functional bug in the new pin gate: the normal `verify_toolkit_checkout()` path referenced an undefined `load_ps3recomp_lock()`, so real analyze/lift/build commands would fail before toolchain use even though the explicit-SHA unit test passed.
- Reconciled the branch by importing the canonical lock loader, retaining exact checkout verification, restoring Python-wheel Ninja discovery and explicit CMake Ninja handoff, retaining byte-preserving PPU patch/audit handling, and combining generic TODO + unsupported-SPR no-op gates.
- Restored regression coverage for Windows CP-1252 generated source and added coverage for the default lock-loading path.
- Preserved the newer native unhandled-exception boot diagnostics already added to `port/main.cpp`.
