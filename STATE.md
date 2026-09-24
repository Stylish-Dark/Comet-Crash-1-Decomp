# STATE

## Current objective

Produce a Windows-native static-recompilation port of **Comet Crash** (PS3, NPEB00142 v1.00) from user-owned game data, preserving vanilla/controller behaviour first and then adding first-class PC input/display improvements without redistributing proprietary title material.

## Current phase

**First native Windows boot reached; second evidenced post-frame blocker isolated and Boot Fix 3 built.**

Boot Fix 2 confirmed the Windows guest-thread-exit crash is fixed. The title continued substantially farther through D3D12 rendering and model/shader/resource loading, then the guest allocator called the title abort reporter. The reported return address was `0x001A4E90`; disassembly shows the actual abort call is the preceding `bl` at `0x001A4E8C` to `0x0019427C`.

## Completed

- Recovered the advanced port tree and restored it as the canonical GitHub source.
- Pinned `ps3recomp` to `d3ed1a5c946a9c5370b51631e13371a1adf70396`.
- Implemented deterministic FREE-NPDRM EBOOT reconstruction and strict title/version/hash validation.
- Reproduced and now hard-gate the known binary-analysis baseline:
  - 3,473 OPD descriptors;
  - 3,409 unique PPU functions;
  - 171 firmware imports across 18 libraries;
  - 24 `cellSpurs` imports;
  - two embedded SPU ELFs with known size/SHA-256 fingerprints.
- Completed the PPU lift and both embedded SPU lifts.
- Added the Comet VMX compatibility fixes for `vsrab` and `vsrb`.
- Added PPU completeness auditing for remaining `TODO` holes and unsupported-SPR no-op fallbacks.
- Added reachable-SPU unsupported-instruction auditing.
- Added exact HLE coverage gating for all 171 firmware imports.
- Implemented Windows D3D12 runner integration plus Comet-specific VFS/HDD, RESC, GCM, SPURS and HLE compatibility work.
- Preserved controller input and added mouse compatibility-mode injection through the original `cellPadGetData` path.
- Added persistent host settings and an F1 Graphics & Input overlay.
- Hardened fresh-machine bootstrap, exact toolchain reset, dependency verification, Ninja discovery and clean analysis/lift/build state.
- Added durable boot logs, stage markers, native crash diagnostics, capped 10/30/60/120/300-second hang snapshots and automatic failure summaries.
- Added `tools/boot_triage.py`, which preserves the first chronological symptom, prefers the first later subsystem-specific signal for classification, and classifies evidence into VM/PPU, VFS, HLE, GCM/RESC/RSX, SPURS/SPU, synchronization, audio, input, or `unknown` without guessing from generic crash/watchdog symptoms.
- Native runs now preserve diagnostics on Ctrl+C and support an optional bounded timeout; timeout escalates terminate → 5-second grace → kill.
- Every completed run writes both the human-readable boot log and a structured `.summary.json` sidecar containing the outcome, first-frame state, signals, subsystem/rationale, timeout/interruption state and exit code.
- Successful builds write `build/build_provenance.json` with the exact tracked-source snapshot, ps3recomp commit, HLE coverage, generated PPU/SPU unit counts, and the actual `CometCrashPC.exe` SHA-256/size. `run` now hard-fails if that executable/source/toolchain provenance no longer matches, unless the explicit `--allow-unprovenanced` diagnostic override is used and recorded.
- Added Windows CI that clones the exact ps3recomp pin, runs the full unit/`compileall`/repository-safety suite on Windows, applies Comet runtime patches, runs the **real pinned PPU lifter** on a non-proprietary PPC fixture, applies the same PPU patch/audit path, compiles/links `CometCrashPC.exe`, and then performs a real linked-EXE provenance round-trip. The CI SPU fixture remains synthetic. Successful runs now publish a clearly marked non-playable CI scaffold artifact plus a **CometCrashPC-Windows-Builder** artifact that builds the real executable locally from the user's own game data.
- Added Linux-hosted Windows cross-compilation using LLVM-MinGW. Actions run `35975062675` cross-built the Windows scaffold successfully from Ubuntu. The compact toolchain kit was then downloaded into the model environment and rehearsed fully offline to a valid PE32+ x86-64 `CometCrashPC.exe`. This removes Visual Studio/Windows SDK installation from the user's build burden.
- Added `tools/verify_boot_bundle.py`; each `.summary.json` now binds to the finalized text log SHA-256 and can independently re-derive/compare the boot outcome, signals, subsystem and provenance status before debugging begins.
- Pinned D3D12 initialization now exposes exact HRESULT-bearing failure lines for nine previously generic/silent setup exits, and triage captures those specific errors before the later generic `D3D12 init FAILED` line.
- First real native boot evidence reached `[boot-stage] first guest frame presented`, controller polling and five frames. It then failed with Windows `STATUS_BAD_FUNCTION_TABLE` (`0xC00000FF`) immediately after `sceNpTerm()` -> `sys_ppu_thread_exit(0)` on a guest worker. The pinned runtime's Windows `longjmp()` thread-exit path was identified as the host failure and replaced with `_endthreadex()` for `_beginthreadex()`-created guest threads; POSIX retains `longjmp()`.
- Boot Fix 2 advanced beyond that host unwind crash and continued loading/rendering resources until the title's allocator path called the guest abort reporter from return address `0x001A4E90`. The actual call instruction is `0x001A4E8C: bl 0x0019427C`. Boot Fix 3 is an explicitly diagnostic build that replaces only that call with a PPC NOP to determine whether the bad-free condition is isolated or evidence of broader heap corruption.
- Latest authoritative validation (GitHub Actions run `35969371258`): **131/131 tests passed**, `compileall` passed, repository safety passed on Linux and Windows, the real pinned-lifter fixture passed, clang-cl/Ninja linked `CometCrashPC.exe`, and the linked Windows EXE provenance verification passed.

## Current working state

The one-command Windows path is:

```bat
scripts\build_and_run.cmd "D:\Games\Comet Crash"
```

It performs toolchain bootstrap, EBOOT reconstruction, strict validation, clean analysis, analysis-baseline verification, clean PPU/SPU lift, compatibility/audit gates, clean native build, and launch.

The native run writes `logs\boot-YYYYMMDD-HHMMSS.txt` plus `logs\boot-YYYYMMDD-HHMMSS.summary.json`. Evidence includes:
- startup/runtime metadata and embedded build provenance;
- last `[boot-stage]` and whether the first guest frame was ever presented;
- first chronological symptom plus the first subsystem-specific `triage_signal` when one appears later;
- evidence-based `suspected_subsystem` and `triage_rationale`;
- timeout/interruption state, host exit code and a coarse `boot_outcome` such as `failure-before-frame`, `clean-visible-exit`, `visible-output-then-failure`, `timed-out`, or `interrupted`.

## Established findings

- Supported title: NPEB00142 v1.00 only.
- Reference rebuilt ELF SHA-256: `3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
- Legacy accepted reconstruction SHA-256: `cf349416cd7f13cf77ac8252ea41c3676d1674496533d2904cf1acd49f516b32`.
- Large embedded SPU image is Sony MultiStream MP3 middleware.
- Small embedded SPU image is active but still unidentified.
- `cellPadGetData` and the downstream pad decoder are mapped.
- Full PPU lift emitted 3,744 functions after boundary recovery/tail wrappers.
- Small SPU: 28 reachable functions, zero reachable unsupported instructions.
- MultiStream SPU: 1,099 reachable functions from entry `0x3050`; 446 unsupported markers are outside the reachable set.
- Direct absolute mouse/world-pointer injection is not yet implemented; current mouse support intentionally uses analogue compatibility mode.

## Unresolved questions

- What is the next runtime blocker after the Windows PPU-thread exit fix, if any?
- What role does the small active SPU program play?
- Which runtime subsystem fails first if the title does not reach visible output: VM/PPU, VFS, HLE, GCM/RESC/RSX, SPURS/SPU, synchronization, audio or input?
- Direct absolute mouse pointer behaviour remains deferred until vanilla gameplay is proven stable.
- Frame-rate unlocking remains deferred until simulation timing is proven stable.

## Blockers

The title archive is available privately and model-side builds are reproducible. The current blocker is runtime evidence from Boot Fix 3: determine whether bypassing the single observed allocator abort lets the title reach stable menu/gameplay or exposes broader heap corruption.

Do not invent the next runtime defect without a boot log.

## Important files

- `WORK_QUEUE.md` — bounded next work units.
- `PROJECT_PLAN.md` — overall architecture/phases/end state.
- `DECISIONS.md` — settled engineering decisions.
- `SESSION_LOG.md` — chronological work-cycle history.
- `docs/STATUS.md` — detailed technical status.
- `docs/reference/known-analysis.json` — binary-analysis baseline.
- `docs/reversing/input.md` — controller/input reversing.
- `docs/reversing/spu.md` — SPU findings.
- `config/ps3recomp.lock` — exact toolchain pin.
- `config/proprietary-inputs.json` — SHA-256/size/storage reference for the private user-owned title archive; no proprietary bytes.
- `tools/comet_port.py` — main pipeline.
- `tools/boot_triage.py` — deterministic saved-boot-log classifier.
- `tools/verify_boot_bundle.py` — integrity/provenance verifier for `.summary.json` + adjacent text log.
- `scripts/build_and_run.cmd` — one-command Windows path.

## Most recent checkpoint

Latest engineering merge on `main`: `b8f4657238144acc03749c0fb2335c8a5cc61376` — **build: support Linux-hosted Windows cross-compilation**.

GitHub Actions run `35972413605` passed **134/134 tests**, Linux repository safety, the Windows unit/safety gate, all pinned runtime patches, real pinned-lifter scaffold staging, native clang-cl/Ninja link, linked-EXE provenance verification, and both artifact uploads.

Published artifacts from that run:
- `CometCrashPC-Windows-Builder` — usable delivery package; exact source bundle + `BUILD_AND_RUN.cmd` + README. Artifact SHA-256: `09752ced9d0f089c980de3b6c5d9161fa4ab2c555b60b4f3df86d01d693f1a68`.
- `CometCrashPC-ci-scaffold` — explicitly non-playable synthetic-fixture validation executable.

## Immediate next action

Run Boot Fix 3 and upload `boot-console.txt` if it exits/crashes. The launcher automatically opens the log in Notepad; no console copying is required.

Repository build path remains:

```bat
scripts\build_and_run.cmd "<path to extracted Comet Crash>"
```

or extract the `CometCrashPC-Windows-Builder` artifact and drag the extracted NPEB00142 v1.00 game folder onto `BUILD_AND_RUN.cmd`.

Preserve both the generated `.txt` log and `.summary.json` sidecar. For a deliberately bounded attempt, `scripts\build_and_run.cmd "<game folder>" 60` applies a 60-second timeout to the native-run phase while still preserving diagnostics. Before acting on the result, run `python tools/verify_boot_bundle.py <boot.summary.json>`. The next AI work unit is to inspect that verified real evidence, fix only the first evidenced blocker, add a focused regression test, update this file and `WORK_QUEUE.md`, and commit.
