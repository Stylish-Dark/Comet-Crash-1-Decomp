# STATE

## Current objective

Produce a Windows-native static-recompilation port of **Comet Crash** (PS3, NPEB00142 v1.00) from user-owned game data, preserving vanilla/controller behaviour first and then adding first-class PC input/display improvements without redistributing proprietary title material.

## Current phase

**First native Windows boot reached; Boot Fix 8 repairs an earlier missed inline PPU jump-table dispatch and is ready for runtime validation.**

Re-analysis of the returned Boot Fix 6 log exposed an earlier control-flow failure that precedes the later allocator abort: guest thread 4 reports an unresolved indirect call to `0x00052DF4` with LR `0x000545DC`. Exact ELF disassembly proves `0x00052DF4` is not a function entry; it is the fifth case target of the seven-entry inline signed-relative jump table immediately following the `bctr` at `0x0005220C` inside `func_0005207C`. The pinned lifter failed to recover that table because its base had been spilled to the stack, so the switch fell through to the global indirect dispatcher, which cannot dispatch interior basic-block labels. PR #15 fixes this generically in the pinned-lifter patch path. A fresh exact-title lift now emits a seven-case C++ switch including `case 0x00052DF4u: goto loc_00052DF4;`, while preserving the exact PPU raw-word baseline and zero actionable PPU holes.

Boot Fix 4 captured the first allocator invariant failure without suppressing it. `mspace_free` receives `mem=0x00000140` / chunk `0x00000138` with a zero header while the same mspace reports `least=0x40000000`. Static tracing shows this value is written to the caller's output buffer from the aligned-allocation result. Boot Fix 6 runtime evidence showed none of the malloc/memalign low-return markers fire before the same `0x140` free, so the allocator is not manufacturing the low value. Static PPC/lift comparison confirms the game stores the allocation in the caller's `sp+0x84` slot and later frees that same slot. Boot Fix 7 now traces the pointer across saved registers, stack-pointer stability, and every concrete call boundary that can intervene before the free. A coverage audit found 13 branch/switch call sites missing from the first build; the hardened build now watches all 35 live-pointer call boundaries that can feed the `0x00130418` free path. It now also checks immediately before the parser output store and immediately before the free, closing straight-line corruption gaps after the final watched call. A fresh exact-title lift also exposed and fixed a separate pipeline bug: the pinned lifter emits 3,936 deterministic data-like `.word` TODO fallbacks, which are now hard-gated by exact count + ordered-value SHA-256 while any real unsupported instruction remains fatal.

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
- Fixed offline artifact continuity: Actions now fetches full Git history, creates self-contained Git bundles from all refs, and smoke-clones each bundle before upload. The prior shallow bundle could contain a commit whose parent was omitted; PR #8 removed that hidden dependency.
- Added Linux-hosted Windows cross-compilation using LLVM-MinGW. Actions run `35975062675` cross-built the Windows scaffold successfully from Ubuntu. The compact toolchain kit was then downloaded into the model environment and rehearsed fully offline to a valid PE32+ x86-64 `CometCrashPC.exe`. This removes Visual Studio/Windows SDK installation from the user's build burden.
- Added `tools/verify_boot_bundle.py`; each `.summary.json` now binds to the finalized text log SHA-256 and can independently re-derive/compare the boot outcome, signals, subsystem and provenance status before debugging begins.
- Boot Fix 5 allocator evidence is now first-class triage data: `boot_triage.py` and live logging capture the low-return stage as `memalign_origin` (`backing-malloc`, `alignment-core`, or `wrapper-return`) plus the exact `memalign_signal`; bundle verification re-derives both from the raw log and rejects tampered summaries.
- Pinned D3D12 initialization now exposes exact HRESULT-bearing failure lines for nine previously generic/silent setup exits, and triage captures those specific errors before the later generic `D3D12 init FAILED` line.
- First real native boot evidence reached `[boot-stage] first guest frame presented`, controller polling and five frames. It then failed with Windows `STATUS_BAD_FUNCTION_TABLE` (`0xC00000FF`) immediately after `sceNpTerm()` -> `sys_ppu_thread_exit(0)` on a guest worker. The pinned runtime's Windows `longjmp()` thread-exit path was identified as the host failure and replaced with `_endthreadex()` for `_beginthreadex()`-created guest threads; POSIX retains `longjmp()`.
- Boot Fix 2 advanced beyond that host unwind crash and continued loading/rendering resources until the title's allocator path called the guest abort reporter from return address `0x001A4E90`. The actual call instruction is `0x001A4E8C: bl 0x0019427C`.
- Boot Fix 3 bypassed that one call, then later hit `mspace_free` assertion `chunksize(p) == small_index2size(I)` and aborted via return address `0x001AB8D8` / allocator site `0x001A4FC4`. This confirms broader allocator-state corruption and invalidates abort suppression as a fix.
- Boot Fix 4 restores the original reference ELF and allocator abort. Generated PPU code is instrumented by `tools/patch_comet_allocator_diag.py` to emit `[COMET-ALLOC-CORRUPTION]` and `[COMET-ALLOC-STATE]` immediately before the first abort, including caller LR, freed pointer, chunk header/size flags, adjacent chunk header, bin maps, dv/top sizes and heap pointers. The returned evidence is `caller_lr=0x001A8190`, `mspace=0x00722220`, `mem=0x00000140`, `chunk=0x00000138`, zero chunk head/size, and `least=0x40000000` — an invalid low pointer, not an ordinary in-heap chunk.
- Added `tools/patch_comet_memalign_diag.py` to instrument the exact regenerated aligned-allocation path at wrapper `0x001A80D0`, core `0x001A75E8`, and its backing `malloc` call. It emits low-result markers only when a nonzero result is below the allocator's `least` address, distinguishing a bad backing allocation from alignment-carving/return corruption.
- Exact disassembly of the reference ELF plus the real generated lift shows `mspace_malloc` (`0x001A5A90`) returns its user pointer through `r31`, and ordinary successful paths form that pointer as `chunk + 8`. The observed `0x00000140` therefore has the structural form of a bogus allocator chunk at `0x00000138`, strongly pointing at poisoned allocator metadata rather than a memalign wrapper arithmetic error.
- Added `tools/patch_comet_malloc_source_diag.py`. It tags all **26 live writes to r31** in `func_001A5A90` and emits `[COMET-MALLOC-LOW]` / `[COMET-MALLOC-STATE]` if the allocator returns below `mspace->least`, identifying the exact basic-block producer plus request, chunk metadata, maps, DV/top state and relevant registers. Guest allocator behaviour is unchanged.
- Boot Fix 6 runtime evidence disproved the low-return hypothesis: no `[COMET-MALLOC-LOW]` or `[COMET-MEMALIGN-*]` marker occurs before the same `mem=0x00000140` failing free. The corruption therefore occurs after successful allocation.
- Exact PPC/lift tracing shows `func_00130130` passes `sp+0x84` to `func_0012F590`, then later reloads `sp+0x84` immediately before the failing free. `func_0012F590` keeps the valid allocation in nonvolatile registers before writing the output. This narrows the fault to saved-register clobber, caller-SP drift, or a later write to the `sp+0x84` slot.
- Added `tools/patch_comet_parse_pointer_diag.py`; Boot Fix 7 instruments the allocation capture, r31/r23 preservation, caller SP, the protected `sp+0x84` slot, and final pre-free state. The hardened watcher covers all **35** live-pointer call boundaries, including 13 branch/switch calls that can loop back into the same final free path. First-failure markers are `[COMET-PARSE-REG-CLOBBER]`, `[COMET-PARSE-SP-CHANGE]`, or `[COMET-PARSE-SLOT-CHANGE]`.
- Closed the remaining straight-line diagnostic gaps with terminal pre-store/pre-free guards and validated their ordering before the actual pointer sinks.
- Repaired the clean real-title PPU completeness gate: 3,936 deterministic `.word` fallbacks are tracked separately from actionable unsupported instructions and pinned by ordered-value SHA-256 `9aceb911a9a8d0fcf45f070935928dbbcbba9f77db083bd5be83a77e03532734`. Any count/digest drift or real mnemonic TODO remains a hard failure.
- Latest authoritative validation (GitHub Actions run `36235777677`, PR #16): **169/169 tests passed**, repository safety passed, Windows scaffold/native-link passed, and Linux→Windows cross-build passed.

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
- Exact reference PPU lift contains 3,936 deterministic data-like `.word` TODO fallbacks; actionable unsupported PPU instructions after compatibility patching are zero. The raw-word ordered-value baseline SHA-256 is `9aceb911a9a8d0fcf45f070935928dbbcbba9f77db083bd5be83a77e03532734`.
- The repaired exact-title lift contains **99 recovered computed jump-table dispatchers**, **694 case occurrences**, and **689 unique case targets**. Their ordered per-dispatcher baseline SHA-256 is `ea920e593b23773631066e58b546c23f71007d145b9d22ffac4a75ea92b1e7b7`; PR #16 makes any structural drift a hard lift failure.
- The Boot Fix 6 log contains an earlier unresolved indirect guest call to `0x00052DF4`. Exact disassembly maps this to case 4 of the inline relative jump table after `0x0005220C: bctr`; table base `0x00052210` plus signed offset `0x00000BE4` equals `0x00052DF4`. The repaired lift now generates this as an in-function case label instead of a global indirect dispatch.
- `func_00130130` contains two frees of its `sp+0x84` parse buffer, but static control flow shows they are mutually exclusive: the long path that reaches the evidenced `0x00130418` failure branches around the earlier `0x001301A8` free. This is not an obvious double-free.
- The long path has no direct caller-side write to `sp+0x84` after parsing. `func_0001C7D4` receives adjacent `sp+0x80`, but with the observed count of 1 its downstream writer touches only `sp+0x80`, not `sp+0x84`.
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

The title archive is available privately and model-side builds are reproducible. The immediate blocker is now runtime validation of Boot Fix 8, because the returned Boot Fix 6 log shows a concrete unresolved control-flow defect before the later `0x00000140` free. Do not assume the allocator failure survives the repaired switch. Boot Fix 8 retains all Boot Fix 7 pointer-lifetime diagnostics so that, if the `0x140` free still occurs, the same run can identify the first saved-register/SP/slot corruption boundary.

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
- `tools/patch_ps3recomp_inline_jumptable.py` — pinned-lifter repair for stack-spilled inline signed-relative PPU jump tables.
- `tools/boot_triage.py` — deterministic saved-boot-log classifier.
- `tools/patch_comet_allocator_diag.py` — first bad-free metadata diagnostic.
- `tools/patch_comet_memalign_diag.py` — aligned-allocation low-return origin diagnostic.
- `tools/verify_boot_bundle.py` — integrity/provenance verifier for `.summary.json` + adjacent text log.
- `scripts/build_and_run.cmd` — one-command Windows path.

## Most recent checkpoint

PR #15 merged to `main` as `2eb2a228ca8d0c9db8da4ff5481ff40c60b5358c` (**recover Comet inline relative PPU jump table**). GitHub Actions run `36232221459` passed **168/168 tests**, repository safety, Windows native-link/provenance and Linux→Windows cross-build.

The fix is applied reproducibly by `tools/patch_ps3recomp_inline_jumptable.py` during the normal lift pipeline. On the exact reference ELF, the previously missed dispatcher at guest `0x0005220C` now decodes seven inline relative targets from table base `0x00052210`, including `0x00052DF4`. The fresh generated C++ contains both the corresponding switch case and `loc_00052DF4`; the former unresolved global-dispatch path for that switch is gone. PPU audit remains zero actionable unsupported instructions with the exact 3,936-entry raw-word baseline.

PR #16 merged to `main` as `5fe95f2289e4e2b2e8ed7ad94c5116b7142927da` (**hard-gate recovered PPU jump tables**). Actions run `36235777677` passed **169/169 tests**, repository safety, Windows scaffold/native-link and Linux→Windows cross-build. The exact-title lift baseline is now pinned at 99 dispatchers / 694 case occurrences / 689 unique targets with digest `ea920e593b23773631066e58b546c23f71007d145b9d22ffac4a75ea92b1e7b7`.

A fresh user-ready Windows Boot Fix 8 package was assembled model-side from the exact repaired title lift with all allocator/memalign/malloc/parse-pointer diagnostics retained. Its launcher supplies `EBOOT.ELF`, sets the title VFS environment, recursively unblocks extracted files, bundles the only non-Windows runtime DLL imports (`libc++.dll` and `libunwind.dll`), and preserves `boot-console.txt` if the process exits.
- EXE SHA-256: `7949faaea3dc33be707f328b512f420932bbcef491ede856db30d824e53041de`.
- Ready-to-run ZIP SHA-256: `31b2512a5f3791497f4293ecfabd9d04ec3a6bc3dbf78d5ecb116d3b51085078`.
- The previously recorded `b945f107...` / `4e16f475...` hashes are an earlier BF8 candidate and are superseded for the current user handoff.

## Immediate next action

Run the ready-to-run **Boot Fix 8 — jump-table fix** package and preserve `boot-console.txt`.

First verify that `[ppu] unresolved indirect call -> 0x00052DF4` is gone. Then:
- if the title advances, debug only the next evidenced blocker;
- if the later `mem=0x00000140` allocator failure still occurs, use the retained `[COMET-PARSE-REG-CLOBBER]`, `[COMET-PARSE-SP-CHANGE]`, and `[COMET-PARSE-SLOT-CHANGE]` markers to identify the exact remaining corruption boundary.

Current user-ready Boot Fix 8 EXE SHA-256: `7949faaea3dc33be707f328b512f420932bbcef491ede856db30d824e53041de`.
Current user-ready Boot Fix 8 ZIP SHA-256: `31b2512a5f3791497f4293ecfabd9d04ec3a6bc3dbf78d5ecb116d3b51085078`.
