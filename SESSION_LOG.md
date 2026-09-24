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
- Tightened title-version safety: `validate_inputs` now rejects unknown EBOOT SHA-256 values and mismatched `VERSION`/`APP_VER` instead of merely warning and continuing into version-specific compatibility code. Both known accepted v1.00 ELF reconstructions remain valid.
- Added unknown-ELF and version-mismatch regressions. Local gate: **74/74 tests passed** plus `compileall`.
- Added `build --clean` support and made the one-command Windows launcher use it, so stale CMake cache/compiler state cannot survive across otherwise-clean bootstrap/lift runs; manual standalone builds remain incremental by default.
- Added parser/launcher regressions. Local gate: **76/76 tests passed** plus `compileall`.
- Functional testing exposed a real regression in `tools/comet_port.py`: `find_ninja` and `load_ps3recomp_lock` were referenced but not imported, which would have broken a real build/default pin check with `NameError`. Restored both imports.
- Added regressions that call the default lock-based checkout verification and assert the runtime helpers are imported/callable, preventing source-only tests from missing this class again. Local gate: **78/78 tests passed** plus `compileall`.
- Added incremental boot-log summarization in `run_logged`: it tracks the last boot stage and first concrete crash/watchdog/unimplemented signal while streaming output, appends them to the log, and prints the summary before raising on a non-zero native exit.
- Added failure-path regression coverage. Local gate: **79/79 tests passed** plus `compileall`.
- Closed strict-validation bypasses: `lift` now validates the supported ELF fingerprint independently, and `run` revalidates the game SFO plus ELF immediately before launching the native process.
- Added direct-lift/direct-run gate regressions. Local gate: **81/81 tests passed** plus `compileall`.

- Restored the exact Ninja executable handoff in `tools/comet_port.py`: `cmd_build` now uses `find_ninja()` and passes the resolved executable to CMake through `CMAKE_MAKE_PROGRAM`, including wheel-only Ninja installs. Added functional regression coverage. Local gate: **82/82 tests passed** plus `compileall`.

- Added `tools/audit_analysis.py` and wired it into `cmd_analyze`: the exact pinned toolkit must reproduce 3,473 OPDs / 3,409 functions, entry/TOC, all 171 imports with the exact 18-library distribution, and the two known SPU images by size+SHA-256 before lifting can begin. Drift now hard-fails. Added focused regressions. Local gate: **86/86 tests passed** plus `compileall`.

- Reconstructed project state from GitHub rather than chat history at commit `a4cbef69f03fce29999a7df48015f7acbc7abaab`.
- Added canonical `STATE.md`, bounded `WORK_QUEUE.md`, and `PROJECT_PLAN.md` handoff documents; converted legacy `CONTINUITY.md`/`NEXT.md` into compatibility pointers.
- Updated README/DECISIONS/technical status to use the new handoff model and current recorded **86/86** regression state.
- Current project bottleneck remains the first real Windows native boot; no new runtime defect is inferred without that evidence.

- Added `tools/boot_triage.py`, a deterministic saved-log classifier for the runtime categories already used by the work queue. It extracts the authoritative footer when present, detects the last boot stage/first signal/exit code/first-frame state, and reports a suspected subsystem plus rationale. Generic crash/watchdog symptoms deliberately remain `unknown` rather than inventing a cause.
- Added 6 focused triage regressions; standalone local gate: **6/6 tests passed** and `compileall` passed for the new module/tests.
- The real first Windows boot remains externally blocked; next cycle should consume the actual log with the triage tool and fix only the first evidenced blocker.

- Integrated `boot_triage.classify_signal` directly into `tools/comet_port.py`'s durable `run_logged` footer and console summary. Native runs now automatically append `suspected_subsystem` and `triage_rationale`; no second command is required for the initial classification.
- Extended boot-log regression coverage for both generic-watchdog=`unknown` and HLE=`hle` cases. A standalone integration harness exercised both paths successfully (**2/2**).
- Engineering checkpoint: `63690db12956f0fc0c6182bccfd6a69e84d9b6c8` (`boot: classify native failures automatically`).

## 2026-09-23 — First-boot evidence pipeline hardened and cross-platform validated

- Changed triage to preserve the first chronological symptom while separately selecting the first subsystem-specific signal; a generic watchdog can no longer hide a later HLE/RSX/SPU signature. Commit: `de067fb`.
- Added durable Ctrl+C handling and optional native-run timeout. Timeout termination escalates through the same terminate -> 5-second grace -> kill path. Fixed the Windows batch forwarding bug found during read-back. Commits: `8803900`, `7927639`, `878cb7f`.
- Added structured `.summary.json` output and explicit boot outcomes, including first-frame state, signals, subsystem/rationale, interruption/timeout state and exit code. Commit: `53353af`.
- Enriched native crash/hang evidence: crash stage, frame count, last HLE breadcrumb, guest-VM AV target/offset, plus capped watchdog snapshots at 10/30/60/120/300 seconds. Commit: `dd06107`.
- Added `build/build_provenance.json` and embedded it into boot evidence: port Git commit/dirty state, exact ps3recomp pin, HLE coverage and generated PPU/SPU unit counts. Commit: `1484521`.
- Opened validation PR #4 to run the full regression suite on Windows before the real-lifter/native-link gate.
- First validation run exposed two real defects:
  - boot-summary footer metadata was being reparsed as live runtime evidence;
  - the Windows pinned lifter emitted host-code-page source containing byte `0x97`, while `audit_ppu_lift.py` incorrectly required UTF-8.
- Fixed both defects on the validation branch. PPU audit now performs reversible Latin-1 byte scanning because its markers are ASCII-only.
- Final GitHub Actions run `35748082438` passed: **118/118 tests**, `compileall`, repository safety, Windows unit suite, all Comet runtime patchers, real pinned PPU lifter fixture/audit, and final clang-cl/Ninja link of `CometCrashPC.exe`.
- PR #4 merged into `main` as `97132306700d999289b1a6503807fbdc767a3570`.
- The next decisive evidence remains the first real Windows run against the user's supported NPEB00142 v1.00 game data.

## 2026-09-24 — Native evidence integrity and D3D12 first-failure diagnostics

- Added an execution-time native-build provenance gate. Build provenance schema v2 now binds the tracked Git source snapshot, exact ps3recomp pin, full 171/171 HLE coverage, generated PPU/SPU counts, and the actual `CometCrashPC.exe` size/SHA-256. A swapped/stale EXE, changed source snapshot, wrong toolchain pin, or incomplete HLE provenance stops `run` before native execution. Commit: `eac714b`.
- Added explicit `--allow-unprovenanced` for deliberate diagnostics only; use of the override is recorded in the boot metadata rather than silently weakening the evidence contract.
- Added `tools/verify_boot_bundle.py`. Boot summaries now carry schema version, finalized text-log SHA-256, ELF SHA-256 and provenance-verification state; the verifier re-triages the adjacent text log and rejects tampered/truncated/mismatched bundles. Commit: `5d3408d`.
- Audited pinned ps3recomp D3D12 initialization and patched nine verified generic/silent failure exits with exact API/HRESULT or Win32 error diagnostics. Triage now captures `[D3D12] ERROR:` as the first graphics-specific signal instead of waiting for the later generic init failure. Commit: `66443b2`.
- Opened PR #5 to validate the combined tree. Linux passed **131/131 tests**, `compileall`, and repository safety.
- Windows passed the same unit/compile/safety gate, all deterministic Comet runtime patches, real pinned PPU lifter fixture/audit, clang-cl/Ninja link, and a new integration step that hashes the actual linked `CometCrashPC.exe` into provenance and successfully re-verifies it against the live Windows checkout/toolchain.
- Authoritative validation: GitHub Actions run `35969371258`.
- PR #5 merged into `main` as `64df668d4a65df3e4237a438b74b81331504f551`.
- The remaining project boundary is still runtime evidence from the user's actual supported NPEB00142 v1.00 title; pre-boot evidence integrity is now materially stronger.

- Corrected the delivery gap: the CI-linked `CometCrashPC.exe` is a non-playable scaffold because it is linked against synthetic PPU/SPU fixtures. The workflow now preserves it only as `CometCrashPC-ci-scaffold` with an explicit warning.
- Added `scripts/portable_builder.cmd` and a `CometCrashPC-Windows-Builder` artifact. The artifact contains an exact Git source bundle and a top-level `BUILD_AND_RUN.cmd`; with the user's local NPEB00142 v1.00 files it creates a clean checkout, runs the full decrypt/analyse/lift/build pipeline, prints the exact real EXE path, and launches it. Commit: `266422b`.

## 2026-09-24 — Windows builder delivery completed

- Corrected an important delivery distinction: the CI-linked EXE is a non-playable synthetic-fixture scaffold and must not be presented as the gameplay-test executable.
- Added a portable Windows builder artifact containing an exact Git source bundle, `BUILD_AND_RUN.cmd`, and README. The script accepts the user's extracted NPEB00142 v1.00 folder, creates a clean checkout, runs the full local decrypt/analyse/lift/build pipeline, prints the resulting real EXE path, and launches it.
- CI now separately uploads `CometCrashPC-ci-scaffold` with an explicit `README-NOT-PLAYABLE.txt`.
- First validation attempt exposed a CRLF-sensitive edit miss in `build_and_run.cmd`; regression coverage caught that the real EXE path was not actually printed. Fixed on PR #6 before delivery.
- Final Actions run `35972413605` passed **134/134 tests** and all Windows compile/provenance/artifact gates. Both artifact uploads succeeded.
- `CometCrashPC-Windows-Builder` artifact id: `10796339939`; SHA-256 `09752ced9d0f089c980de3b6c5d9161fa4ab2c555b60b4f3df86d01d693f1a68`.
- PR #6 merged into `main` as `6ea1d6f68e5e2f37548a7af7f4da85f1467687c6`.
