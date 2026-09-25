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

## 2026-09-24 — Model-side Windows cross-build established

- Moved the build path model-side after rejecting Visual Studio/toolchain setup as a user requirement.
- Added an LLVM-MinGW CMake toolchain and compiler-conditional port flags while retaining the existing Windows clang-cl path.
- Actions run `35975062675` successfully cross-built the Windows scaffold from Ubuntu.
- Packaged and downloaded a compact offline kit containing the exact source bundle, pinned ps3recomp bundle, compressed LLVM-MinGW toolchain, and Python 3.13 wheelhouse.
- Rehearsed the kit locally with networking unnecessary: repositories reconstructed, dependencies installed from local wheels, Comet patches applied, pinned lifter fixture generated, and a valid PE32+ x86-64 `CometCrashPC.exe` produced.
- Therefore the real EXE can now be built entirely model-side once `PARAM.SFO` and `USRDIR/EBOOT.BIN` from the supported title are supplied.
- PR #7 merged as `b8f4657238144acc03749c0fb2335c8a5cc61376`.

## 2026-09-24 — Proprietary title input persisted without Git duplication

- Received the user's NPEB00142 archive (`190862297` bytes; SHA-256 `d0b9d09a1bdcca45a06bf781330adfe19dab8fd78f986c2312aaa69e6f65e2dd`).
- Persisted exactly one private copy at `/Projects/Comet Crash/private-inputs/Comet-Crash-NPEB00142-v1.00.rar`.
- Added `config/proprietary-inputs.json` as the repository-visible content-addressed reference. The public Git repository stores only filename/size/SHA-256/private-storage path, never the proprietary archive bytes.
- Extended `.gitignore` to block RAR/private-input directories, preventing accidental future commits.
- Future sessions should retrieve the private archive by the manifest path, verify SHA-256, and reuse the same object rather than asking for or uploading another copy.

## 2026-09-24 — First real native boot: five frames reached; Windows unwind crash fixed

- User ran the first real NPEB00142 native package and returned `boot-console.txt`.
- The title successfully reserved guest VM, loaded the exact PPU ELF, initialized HLE/sysPrx/VFS/LV2, initialized SPURS/audio/D3D12, loaded game assets/shaders, began controller polling and emitted `[boot-stage] first guest frame presented`.
- Crash evidence: five frames had been presented; immediately before failure, worker thread 5 called `sceNpTerm()` then `sys_ppu_thread_exit(tid=5,status=0)`. Windows raised `0xC00000FF STATUS_BAD_FUNCTION_TABLE`.
- Root cause identified in pinned ps3recomp: Windows guest thread exit used `longjmp()` across deep recompiled/COFF frames. LLVM-MinGW/Windows unwind metadata rejected that unwind.
- Fix: for Windows only, after `sys_ppu_thread_exit` has recorded status and signalled joiners, terminate the matching CRT thread with `_endthreadex(0)`; POSIX keeps `longjmp()`. Local real-title incremental cross-build linked successfully with EXE SHA-256 `547e71e331385ecc17f245763eab77129ed43d60099293c8533e4211ae4a9029`.
- Boot Fix 2 launcher removes the blocking `pause`; on process exit it automatically opens `boot-console.txt` in Notepad for easy upload/copy.

## 2026-09-25 — Boot Fix 2 advanced; allocator abort isolated; Boot Fix 3 built

- User returned the Boot Fix 2 log. The earlier Windows `STATUS_BAD_FUNCTION_TABLE` thread-exit failure did not recur, confirming the `_endthreadex()` fix cleared that blocker.
- The title continued substantially farther through rendering and resource/model/shader loading.
- New evidenced failure: the guest's own abort reporter was invoked with return address `0x001A4E90`. PPC disassembly identified the real call instruction as `0x001A4E8C: bl 0x0019427C`.
- The surrounding function is the title allocator/free path. Multiple allocator consistency checks converge on that abort site.
- Built an explicitly diagnostic Boot Fix 3 by replacing only the call at guest `0x001A4E8C` (`4B FE F3 F1`) with PPC NOP `60 00 00 00`. Original reference ELF SHA-256 remains the input fingerprint; diagnostic patched ELF SHA-256 is `f0f4ec0b1c1d8a673335964cb64db556f33485019631bf685e56407f0f022bb9`.
- Re-lifted the full real title: 3,744 PPU functions, VMX `vsrab`/`vsrb` compatibility fixes applied, both real SPU images lifted. Generated PPU source shows `loc_001A4E8C` falls through as NOPs.
- Preserved the Boot Fix 2 Windows thread-exit fix, linked against built-in `XINPUT9_1_0.dll`, and added an explicit static `sys_prx_register_library` compatibility registration.
- Linux-hosted Windows cross-link completed successfully. Boot Fix 3 EXE SHA-256: `3f46d2c9eb9280bc6ef92b281a8a802d417cfc8762915496a8b41f56df8f7485`. Ready-to-run ZIP SHA-256: `79489db16e7541153a2e57896eb7aff31c997065e3270614fffd830935e49c72`.

## 2026-09-25 — Boot Fix 3 proves broader heap corruption; Boot Fix 4 diagnostic built

- User returned the Boot Fix 3 log. The prior Windows thread-exit fault remains cleared and the title again reaches D3D12, first frame, controller/save/trophy/NP initialization and extensive resource loading.
- Because Boot Fix 3 suppressed only the first allocator abort, execution advanced until Dinkumware `mspace_free` hit `chunksize(p) == small_index2size(I) -- assertion failed`.
- The later abort chain includes return address `0x001AB8D8`, allocator site `0x001A4FC4`, `free` wrapper `0x001A8190`, and call path through `0x0015625C -> 0x000C6030 -> 0x00109650`. This proves the first abort was not an isolated harmless free; allocator state is genuinely inconsistent.
- Retired the Boot Fix 3 abort suppression as a debugging direction.
- Added optional `tools/patch_comet_allocator_diag.py`, which instruments generated `func_001A4C6C` but preserves the original guest abort. It captures original mspace/mem/caller LR and emits chunk head/size/pinuse/cinuse, next chunk header, small/tree maps, dv/top sizes, least address, dv and top pointers.
- Built Boot Fix 4 from the exact reference ELF SHA-256 `3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6` with the Windows thread-exit fix retained. EXE SHA-256 `384e04af8924e50b552aad5b7b4811f88dd72a9602a09076a36155ecf477413b`; ready-to-run ZIP SHA-256 `6d82cbb3a401280b5a422ed1bb9d3462e8e0227aa93047097e3aa103245ba825`.
- Next evidence required: the first `[COMET-ALLOC-CORRUPTION]` and `[COMET-ALLOC-STATE]` lines from Boot Fix 4.

## 2026-09-25 — Boot Fix 4 isolates invalid low-pointer free

- User returned the Boot Fix 4 log. The diagnostic fired at the original first allocator abort: `caller_lr=0x001A8190`, `mspace=0x00722220`, `mem=0x00000140`, derived chunk `0x00000138`, zero chunk head/size, while the allocator reports `least=0x40000000`, `dv=0x43C80568`, and `top=0x4557A698`.
- This rules out an ordinary in-heap chunk merely having damaged size flags: the pointer itself is outside the allocator's heap range.
- Regenerated the exact reference PPU lift from ELF SHA-256 `3b4b6fef...` using pinned ps3recomp `d3ed1a5c...` and traced the producer statically.
- The failing free wrapper is `0x001A8178` -> `mspace_free`; its caller `0x00130130` frees the first field of an output structure populated by `0x0012F590`.
- `0x0012F590` obtains that buffer from aligned allocator wrapper `0x001A80D0` (alignment `0x80`) and retains that allocation result for the output pointer. Therefore the next diagnostic must observe the allocation return path, not suppress or guard `free()`.
- Added `tools/patch_comet_memalign_diag.py` plus regression coverage. It instruments wrapper `0x001A80D0`, core `0x001A75E8`, and the core's backing `malloc` call, emitting diagnostics only for nonzero returns below the allocator's `least` address.

- First real Boot Fix 5 compile rejected the new memalign diagnostic because a late C++ local declaration sat after guest labels/gotos. Corrected the patch by declaring the diagnostic scratch variable at function entry and assigning it at the backing-malloc call site; this preserves generated control-flow legality.

- Boot Fix 5 private build completed successfully after removing a stray local anchor-test translation unit from the CMake glob and correcting the diagnostic's C++ goto-scope issue in GitHub first.
- Real title PPU lift: 3,744 functions; VMX `vsrab`/`vsrb` fixes applied; both embedded SPUs re-lifted from the exact reference ELF.
- Runtime compatibility retained: Windows `_endthreadex` guest-thread exit fix, built-in `XINPUT9_1_0`, and static `sys_prx_register_library` handler.
- Boot Fix 5 EXE SHA-256: `d05bf3d6672c2105bd223cce0096a259f6822ca94a1911efdae9bfbc4c5e2bb8`.
- Ready-to-run Boot Fix 5 ZIP SHA-256: `5d9cfcfa14a42d79e38badd76973958309664924c7f37a92d7db7df99ee736d8`.
- Next evidence: run Boot Fix 5 and upload `boot-console.txt`; inspect the first of `[COMET-MEMALIGN-MALLOC-LOW]`, `[COMET-MEMALIGN-CORE-LOW]`, or `[COMET-MEMALIGN-WRAPPER-LOW]`.

## 2026-09-25 — Canonical Boot Fix 5 rebuild and offline-bundle repair

- User explicitly reaffirmed that GitHub must be the canonical project state; reconstructed work from `STATE.md` / `WORK_QUEUE.md` before continuing.
- Found a continuity defect in the previously published offline source bundles: Actions' shallow checkout allowed a bundled commit to reference an omitted parent, so a fresh `git clone <bundle>` could fail.
- PR #8 changed all artifact-producing checkouts to full history, creates source bundles with `git bundle --all`, and smoke-clones/verifies them before upload. It also ensures the Linux cross-build applies the Windows PPU-thread-exit patch.
- Validation run `36096273445`: **144/144 tests**, compileall/repository safety, Windows scaffold/native-link/provenance, Linux→Windows cross-build, and both source-bundle smoke clones passed.
- PR #8 merged as `31c8552afd8ff977a535d92a373e0baab2ba5a23`.
- Downloaded the corrected cross-build artifact, successfully cloned its self-contained Comet source bundle and pinned ps3recomp bundle, and checked out Boot Fix 5 diagnostic commit `31024b8193c95a3d432b3cfdc291168991fc088d`.
- Rebuilt Boot Fix 5 from exact reference ELF `3b4b6fef...`: full real PPU lift with `vsrab`/`vsrb` fixes, Boot Fix 4 allocator diagnostic, aligned-allocation wrapper/core/backing-malloc diagnostics, and both real SPU workloads.
- Cross-link completed successfully. Rebuilt EXE SHA-256: `e0986be50ad7c0d3d16c93e9c2a542bbe0c1b52631be03a2ce8b2d08971db1d4`. Ready-to-run ZIP SHA-256: `ec6121dd22e3f035fb8a9e4443e1522c125494a675f35b1fd99e9d0374e21c55`.
- The EXE was checked for all five diagnostic strings: MEMALIGN-MALLOC-LOW, MEMALIGN-CORE-LOW, MEMALIGN-WRAPPER-LOW, ALLOC-CORRUPTION and ALLOC-STATE.


## 2026-09-25 — Boot Fix 5 evidence classification hardened

- Continued from canonical GitHub state with Boot Fix 5 still awaiting one real Windows run; no new runtime defect was invented without evidence.
- Added first-class memalign triage for `[COMET-MEMALIGN-MALLOC-LOW]`, `[COMET-MEMALIGN-CORE-LOW]`, and `[COMET-MEMALIGN-WRAPPER-LOW]`.
- Boot logs and `.summary.json` now record `memalign_origin` (`backing-malloc`, `alignment-core`, or `wrapper-return`) and the exact `memalign_signal`.
- `verify_boot_bundle.py` now re-derives those fields from the raw log, so an altered/misclassified summary is rejected.
- Added focused regression coverage for causal-stage precedence, live logging/sidecar output, and summary tamper detection.
- PR #9 merged as `97c732d2a76f2a942a419e33509abcbc82710a87`.
- GitHub Actions run `36097615167`: **148/148 tests passed**; compileall/repository safety, Windows unit/native-link/provenance, and Linux→Windows cross-build all passed.
- Runtime blocker remains deliberately unchanged: one Boot Fix 5 Windows run is required to determine where the invalid low `0x140` allocation result is first produced.


## 2026-09-25 — Boot Fix 6: exact mspace_malloc return producer traced

- Recovered the user's canonical private NPEB00142 v1.00 archive from project Library and verified the exact reference ELF SHA-256 `3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
- Reconstructed the pinned source/toolchain locally from the self-contained CI bundles and regenerated the real PPU lift for direct inspection.
- Exact ELF disassembly and generated C++ confirm `func_001A5A90` is the title's `mspace_malloc`; successful allocator paths return through `r31`, commonly as `chunk + 8`. The observed bad user pointer `0x00000140` therefore corresponds structurally to a bogus chunk pointer `0x00000138`.
- Added `tools/patch_comet_malloc_source_diag.py`, which tags all 26 live writes to `r31` in `func_001A5A90` and logs the exact producer block, request, chunk header/links, bin maps, DV/top state and relevant registers when a nonzero return is below `mspace->least`.
- Extended boot triage/summary verification with `malloc_source` and `malloc_signal`; altered summaries are rejected when they disagree with the raw boot log.
- PR #10 merged as `d5f51b36ae827cad6cd31846f96c4d35fa58aacd`.
- GitHub Actions run `36099036501`: **154/154 tests passed**; repository safety, Windows native-link/provenance and Linux→Windows cross-build all passed.
- Built a real Windows Boot Fix 6 executable from the exact reference ELF with Boot Fix 4 free diagnostics, Boot Fix 5 memalign diagnostics and the new malloc-source diagnostic all embedded.
- Boot Fix 6 EXE SHA-256: `202788a1ba26d4164bc4a598608c7809d8b5cf02202e02c9ade435cb85da08be`.
- Ready-to-run ZIP SHA-256: `f6697e3a8c03574a853cd7318bb7beef26ed40a22c4b1f755edc9c9254ec26b1`.
- Next runtime evidence is no longer merely “does backing malloc return low?”; it will identify the precise allocator branch that selected the poisoned chunk, allowing the corruption writer to be traced instead of guessed.


## 2026-09-25 — Boot Fix 6 disproves low allocator return; Boot Fix 7 traces pointer lifetime

- User returned the Boot Fix 6 runtime log. The title again reached first guest frame and deep resource loading, then hit the same first bad free: `mem=0x00000140`, chunk `0x00000138`, allocator `least=0x40000000`.
- None of the Boot Fix 5/6 low-result markers fired before the bad free: no `[COMET-MEMALIGN-MALLOC-LOW]`, `[COMET-MEMALIGN-CORE-LOW]`, `[COMET-MEMALIGN-WRAPPER-LOW]`, or `[COMET-MALLOC-LOW]`. This falsifies the prior hypothesis that mspace_malloc/memalign directly returns `0x140`.
- Exact PPC and generated-lift comparison confirms caller `func_00130130` passes `sp+0x84` as the output field to `func_0012F590`, and later reloads the same `sp+0x84` immediately before the failing `func_001A8178` free.
- `func_0012F590` receives a normal aligned allocation, retains it in nonvolatile registers, and eventually stores it to the output field. The remaining failure classes are therefore saved-register clobber, caller stack-pointer drift, or a later overwrite of the caller's `sp+0x84` slot.
- Added `tools/patch_comet_parse_pointer_diag.py`: it checks r31 immediately after `0x0012F6B8`, r23 after five subsequent parser calls, caller r1 plus the protected slot after 22 concrete call boundaries, and final state immediately before the free.
- Added parse-corruption triage fields (`parse_corruption_kind`, `parse_corruption_site`, `parse_corruption_signal`) and boot-bundle tamper verification.
- Stabilized the existing subprocess timeout regression test from 0.2s to 0.75s to avoid racing Python process startup under load.
- PR #11 merged as `99342173036be4db1ac57ff816bfaf56290cfe51`.
- GitHub Actions run `36120069200`: **160/160 tests passed**; repository safety, Windows native-link/provenance, and Linux→Windows cross-build all passed.
- Built Boot Fix 7 locally from the exact reference ELF plus all prior allocator diagnostics and the new pointer-lifetime probe.
- Boot Fix 7 EXE SHA-256: `a599fd666cf672357b35aa45d14e31931c1eaa1c1e1cd3ae8b2a5eb8ec2c1676`.
- Ready-to-run Boot Fix 7 ZIP SHA-256: `a2d873793dc407b7a6372aad943768d62d696bfd491daa72fc905992692c0897`.


## 2026-09-25 — Boot Fix 7 hardened to full live-pointer call coverage

- Audited the real generated `func_00130130` after the first Boot Fix 7 build and found a concrete diagnostic coverage gap: 13 branch/switch call sites at `0x00130440..0x00130548` can loop back into the same `0x00130418` free path but were not included in the original 22-call watcher set.
- Expanded `tools/patch_comet_parse_pointer_diag.py` from 22 to **35** caller-side watchpoints so every known live-pointer call boundary feeding the failing free is checked for caller-SP drift and `sp+0x84` slot mutation.
- Added a regression locking the exact 35-site set; the full repository suite passed **161/161 tests** locally.
- Incrementally cross-linked the real Windows executable after applying the 13 missing watchpoints to the exact generated real-title PPU source.
- Hardened Boot Fix 7 EXE SHA-256: `33d166770570791c7fb4c2151a3a29b52234f71750e1445f2223bf77b23d1545`.
- Hardened ready-to-run ZIP SHA-256: `d2d106e767490998e7ec37b025f2d8186c7381433d9bf3a1ce6686d8d0569d3b`.
- Next evidence remains the first `[COMET-PARSE-REG-CLOBBER]`, `[COMET-PARSE-SP-CHANGE]`, or `[COMET-PARSE-SLOT-CHANGE]` marker from the hardened build; that exact call site will determine the next code fix.


## 2026-09-25 — Boot Fix 7 terminal lifetime gaps closed

- Audited the hardened 35-call pointer-lifetime watcher for non-call straight-line gaps.
- Added a terminal `r23` guard immediately before the parser output store and terminal caller-SP / `sp+0x84` guards immediately before the failing free.
- The terminal checks reuse the existing `[COMET-PARSE-REG-CLOBBER]`, `[COMET-PARSE-SP-CHANGE]`, and `[COMET-PARSE-SLOT-CHANGE]` markers, so late corruption remains machine-classifiable by the existing triage path.
- Updated the focused regression test to lock the extra marker sites and counts.
- Code commit: `7420f2ddaeb99121b0ce23cd0b366ee02e8ffd87`; test commit: `359342bffd02c507ca2a8c06512a80be265c5a56`.
- Next runtime dependency remains one hardened Boot Fix 7 run; no allocator/free suppression was introduced.


## 2026-09-25 — Terminal Boot Fix 7 validated; real-title lift gate repaired

- PR #12 merged as `886573c84500d653ddf77451ff96e8c427560c53`. GitHub Actions run `36124843328` passed **162/162 tests**, repository safety, Windows native-link/provenance and Linux→Windows cross-build.
- Added ordering regression coverage proving the terminal pre-store/pre-free pointer guards execute before their actual pointer sinks.
- Rebuilt the real Windows diagnostic from the exact title lift with the terminal guards included and all prior allocator diagnostics retained.
- Terminal-guard EXE SHA-256: `4f40b8d966cf99cb112379cdf49512af5043b2adfa162485f6e6bbbd03e69b69`.
- Ready-to-run terminal-guard ZIP SHA-256: `f7efd390fd80631b2925e5cc645dc1a72372b8281199b1b57ed6d222e69ddf86`.
- Static review of `func_00130130` found two reads/frees of `sp+0x84`: `0x001301A8` on the short/error path and `0x00130418` on the long path. Control flow makes them mutually exclusive; the long path that reaches the evidenced failure branches around the early free, so the current evidence is not explained by a simple double-free.
- No direct write to `sp+0x84` exists in the caller after parse return. The adjacent `sp+0x80` buffer passed to `func_0001C7D4` was audited; with count=1 its downstream write touches only `sp+0x80`, not `sp+0x84`.
- A fresh exact-reference lift exposed a separate reproducibility bug: the pinned PPU lifter emits 3,936 deterministic data-like `.word` TODO fallbacks, while the old completeness gate treated every TODO as an unsupported instruction. There are zero remaining actionable mnemonic TODOs after the Comet compatibility patch.
- Added strict raw-word baseline handling: expected count **3,936** and ordered-value SHA-256 `9aceb911a9a8d0fcf45f070935928dbbcbba9f77db083bd5be83a77e03532734`. Count/digest drift still fails; real instruction TODOs remain fatal.
- Fresh exact-title end-to-end lift after the fix: **3,744 PPU functions**, zero actionable PPU unsupported TODOs, exact raw-word baseline, both SPU lifts complete, zero reachable unsupported SPU instructions.
- The current Boot Fix 7 pointer patcher applied cleanly to that fresh real-title lift; the resulting 28.9 MB generated C++ compiled successfully for Windows.
- PR #13 merged as `f9069a2e68ada5e5598977780d74f6a4c6070555`. GitHub Actions run `36127073310` passed **164/164 tests**, repository safety, Windows native-link/provenance and Linux→Windows cross-build.
- Next runtime dependency remains the first decisive parse-pointer marker from the terminal-guard Boot Fix 7 build. No allocator/free suppression was introduced.
