# STATE

## Current objective

Produce a Windows-native static-recompilation port of **Comet Crash** (PS3, NPEB00142 v1.00) from user-owned game data, preserving vanilla/controller behaviour first and then adding first-class PC input/display improvements without redistributing proprietary title material.

## Current phase

**First native Windows boot / runtime blocker discovery.**

The static-analysis, lift, compatibility, reproducibility and pre-boot diagnostic work is sufficiently mature that the next high-value evidence is a real Windows run against the supported NPEB00142 v1.00 title.

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
- Added `tools/verify_boot_bundle.py`; each `.summary.json` now binds to the finalized text log SHA-256 and can independently re-derive/compare the boot outcome, signals, subsystem and provenance status before debugging begins.
- Pinned D3D12 initialization now exposes exact HRESULT-bearing failure lines for nine previously generic/silent setup exits, and triage captures those specific errors before the later generic `D3D12 init FAILED` line.
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

- What is the first real runtime blocker on Windows, if any?
- What role does the small active SPU program play?
- Which runtime subsystem fails first if the title does not reach visible output: VM/PPU, VFS, HLE, GCM/RESC/RSX, SPURS/SPU, synchronization, audio or input?
- Direct absolute mouse pointer behaviour remains deferred until vanilla gameplay is proven stable.
- Frame-rate unlocking remains deferred until simulation timing is proven stable.

## Blockers

The principal blocker is external runtime evidence: **a real Windows native boot against the user's extracted supported game data has not yet been captured.**

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
- `tools/comet_port.py` — main pipeline.
- `tools/boot_triage.py` — deterministic saved-boot-log classifier.
- `tools/verify_boot_bundle.py` — integrity/provenance verifier for `.summary.json` + adjacent text log.
- `scripts/build_and_run.cmd` — one-command Windows path.

## Most recent checkpoint

Latest validated engineering merge on `main`: `64df668d4a65df3e4237a438b74b81331504f551` — **ci: validate provenance and first-boot diagnostics**.

GitHub Actions run `35969371258` is fully green at **131/131 tests** plus Linux/Windows safety/compile gates, pinned-lifter staging, native clang-cl/Ninja link, and linked-EXE provenance verification.

## Immediate next action

Run the supported title on Windows:

```bat
scripts\build_and_run.cmd "<path to extracted Comet Crash>"
```

Preserve both the generated `.txt` log and `.summary.json` sidecar. For a deliberately bounded attempt, `scripts\build_and_run.cmd "<game folder>" 60` applies a 60-second timeout to the native-run phase while still preserving diagnostics. Before acting on the result, run `python tools/verify_boot_bundle.py <boot.summary.json>`. The next AI work unit is to inspect that verified real evidence, fix only the first evidenced blocker, add a focused regression test, update this file and `WORK_QUEUE.md`, and commit.
