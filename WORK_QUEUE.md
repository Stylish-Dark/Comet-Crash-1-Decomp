# WORK QUEUE

Work units are intentionally bounded so one worker can investigate, integrate, document and commit without approaching the execution limit.

## Current

[x] **Capture first native Windows boot evidence**
- Input archive is now persisted once in private project storage and referenced by `config/proprietary-inputs.json`. Verify its SHA-256 before extraction; do not re-upload or commit the ROM on future source updates.
- Run `scripts\build_and_run.cmd "<game folder>"`. If a bounded attempt is preferable, pass a native-run timeout as the second argument, e.g. `scripts\build_and_run.cmd "<game folder>" 60`.
- Preserve both `logs\boot-*.txt` and the adjacent `logs\boot-*.summary.json`.
- Confirm the summary contains build provenance, `boot_outcome`, first-frame state, `first_signal`, `triage_signal`, `suspected_subsystem`, `triage_rationale`, timeout/interruption state and exit code.
- Run `python tools/verify_boot_bundle.py <boot.summary.json>`; do not debug from an artifact that fails its log/provenance integrity checks.
- Success for this work unit: a self-verified, provenance-bound boot artifact that identifies how far the native title actually reached and the first concrete runtime blocker, if any.
- Delivery is now available: `CometCrashPC-Windows-Builder` from validated Actions run `35972413605` creates a clean source checkout and runs the real local game-data pipeline.
- No Windows toolchain is required from the user. The title archive is available privately; the remaining work is model-side extraction/validation/build and then user-side execution of the finished EXE.

## Current

[x] **Re-test the first evidenced native blocker fix**
- First real run reached D3D12, first guest frame, controller polling and five frames.
- Crash: `0xC00000FF STATUS_BAD_FUNCTION_TABLE` immediately after `sceNpTerm()` -> `sys_ppu_thread_exit(0)` on guest thread 5.
- Fix: Windows guest threads created with `_beginthreadex()` now terminate with `_endthreadex()` instead of `longjmp()` after exit state/join signalling; POSIX retains the existing jump unwind.
- Run Boot Fix 2 and upload the automatically opened `boot-console.txt` if it exits/crashes.
- Result: `STATUS_BAD_FUNCTION_TABLE` is gone. Boot Fix 2 ran farther and hit a title-side allocator abort instead.

## Current

[x] **Test Boot Fix 3 allocator-abort diagnostic**
- Boot Fix 2 reaches much farther through resource/model/shader loading.
- New failure: title abort reporter called from return address `0x001A4E90`; actual branch-and-link is `0x001A4E8C -> 0x0019427C`.
- Boot Fix 3 replaces only that one branch-and-link with a PPC NOP. This is diagnostic, not accepted permanent gameplay behaviour.
- Result: allocator corruption recurred later in `mspace_free` with assertion `chunksize(p) == small_index2size(I)`. Therefore the bypass is not a valid fix and broader heap inconsistency is confirmed.

## Current

[x] **Capture first corrupt-free metadata with Boot Fix 4**
- Restore the exact reference ELF and original allocator abort; do not suppress allocator assertions.
- Instrument generated `func_001A4C6C` with `tools/patch_comet_allocator_diag.py`.
- Required log markers: `[COMET-ALLOC-CORRUPTION]` and `[COMET-ALLOC-STATE]`.
- Use caller LR, freed pointer, chunk header/flags, adjacent header and mspace state to distinguish invalid pointer/double-free from earlier heap overwrite.
- Result: first failing free receives `mem=0x00000140` / chunk `0x00000138` with zero metadata while allocator `least=0x40000000`. Static trace shows this low value comes from the aligned-allocation result stored by `func_0012F590`; the output field is not the source of corruption.

## Current

[x] **Build Boot Fix 6 and trace the exact low mspace_malloc return producer**
- Keep the exact reference ELF and original allocator abort; retain the Boot Fix 4 first-free diagnostic.
- Boot Fix 5 memalign instrumentation remains enabled. Boot Fix 6 adds `tools/patch_comet_malloc_source_diag.py`, which tags all 26 live `r31` producers in `func_001A5A90`.
- Capture `[COMET-MEMALIGN-MALLOC-LOW]`, `[COMET-MEMALIGN-CORE-LOW]`, and/or `[COMET-MEMALIGN-WRAPPER-LOW]`.
- The runner/triage path now records `memalign_origin` as `backing-malloc`, `alignment-core`, or `wrapper-return`, plus the exact `memalign_signal`; `verify_boot_bundle.py` independently re-derives these fields from the raw text log.
- Inputs of interest: wrapper caller LR, alignment, requested bytes, backing-malloc request, mspace/least, returned pointer, and core working registers.
- Static work already narrows the shape: `0x140 == 0x138 + 8`, matching a normal dlmalloc `chunk + 8` return from a poisoned chunk pointer. Boot Fix 6 identifies exactly which allocator path selected that chunk.
- Boot Fix 6 EXE SHA-256: `202788a1ba26d4164bc4a598608c7809d8b5cf02202e02c9ade435cb85da08be`; ready-to-run ZIP SHA-256: `f6697e3a8c03574a853cd7318bb7beef26ed40a22c4b1f755edc9c9254ec26b1`.
- Success: capture `[COMET-MALLOC-LOW] source=...` and use that exact producer to trace the corrupted bin/DV/tree/top metadata back to its writer.

## Current

[x] **Run Boot Fix 6 and rule out low allocator returns**
- Result: no `[COMET-MALLOC-LOW]` or `[COMET-MEMALIGN-*]` marker appeared before the same `0x140` free. The low pointer is introduced after successful allocation, not by dlmalloc/memalign.

## Current

[ ] **Run Boot Fix 7 and identify the first pointer-corruption boundary**
- Trace the allocation from `func_0012F590` through saved `r31/r23`, the caller's `r1`, and the caller's `sp+0x84` lifetime slot.
- First decisive marker wins: `[COMET-PARSE-REG-CLOBBER]`, `[COMET-PARSE-SP-CHANGE]`, or `[COMET-PARSE-SLOT-CHANGE]`.
- Use the reported `site=0x...` / callee to patch the exact offender.
- Boot Fix 7 EXE SHA-256: `a599fd666cf672357b35aa45d14e31931c1eaa1c1e1cd3ae8b2a5eb8ec2c1676`.
- Boot Fix 7 ZIP SHA-256: `a2d873793dc407b7a6372aad943768d62d696bfd491daa72fc905992692c0897`.

## Next

[x] **Verify triage and fix the first evidenced native blocker**
- Use the `.summary.json` plus the full text log; `tools/boot_triage.py` remains available for independent re-analysis. Do not speculate ahead.
- Classify into VM/PPU, VFS, HLE, GCM/RESC/RSX, SPURS/SPU, synchronization, audio, or input.
- Make the smallest faithful fix.
- Add a focused regression test or deterministic audit.
- Re-run relevant gates and checkpoint.

[ ] **Verify stable visible output / menu rendering**
- Confirm D3D12 initialization.
- Confirm at least one presented guest frame.
- Verify title/menu rendering stability.
- Record runtime evidence in `STATE.md` and `SESSION_LOG.md`.

[ ] **Verify vanilla controller gameplay**
- Confirm original controller mappings.
- Enter a mission.
- Verify simulation, placement/selection, pause, exit, save/load and clean shutdown.
- Fix only observed regressions.

[ ] **Resolve required SPU/audio behaviour**
- Determine whether the small active SPU workload is required for gameplay.
- Restore/fix audio/SPURS behaviour based on runtime evidence.
- Keep gameplay behaviour faithful; no approximate host-side replacement for core logic.

## Later

[ ] Native absolute mouse/world-pointer injection.
[ ] Configurable keyboard bindings beyond compatibility-mode mapping.
[ ] Safe aspect-ratio/display improvements.
[ ] High-refresh/frame-pacing work after simulation timing is proven.
[ ] Packaging/user-facing validation polish.
[ ] Network-service work only if required for local gameplay goals.

## Completed in the latest continuity cycle

[x] Restore advanced recovered source tree and make GitHub canonical.
[x] Harden exact ps3recomp pin/bootstrap/dependency handling.
[x] Add strict title/version/hash gates.
[x] Add deterministic analysis-baseline gate.
[x] Add PPU/SPU/HLE completeness gates.
[x] Add Windows real-lifter compile/link CI gate.
[x] Add native crash, hang and boot-summary diagnostics.
[x] Establish `STATE.md`, `WORK_QUEUE.md` and `PROJECT_PLAN.md` as the durable handoff structure.
[x] Add deterministic saved-boot-log subsystem triage with conservative `unknown` handling for generic crash/watchdog symptoms.
[x] Integrate triage directly into `comet_port.py run` and prefer later subsystem-specific evidence over an earlier generic watchdog.
[x] Preserve boot diagnostics across optional timeout and Ctrl+C; bound timeout termination with kill escalation.
[x] Emit structured `.summary.json` boot outcomes and embed exact build provenance.
[x] Enrich native crash/watchdog evidence with stage, frames, HLE breadcrumb and guest-VM AV location.
[x] Run the full 118-test suite on Linux and Windows CI; verify real pinned PPU lifter staging and clang-cl/Ninja native scaffold link.
[x] Fix Windows host-encoding drift in generated PPU auditing and prevent footer metadata from being reparsed as runtime signals.
[x] Enforce source/toolchain/HLE/native-EXE provenance before launch, with only an explicit recorded diagnostic override.
[x] Add self-verifying boot bundles that hash the finalized text log and re-derive the saved triage/outcome fields.
[x] Add exact HRESULT diagnostics for nine previously generic/silent D3D12 initialization exits.
[x] Run the expanded **131/131** Linux/Windows gate, including Windows repository safety, real pinned-lifter staging, native link, and linked-EXE provenance round-trip.
[x] Publish a downloadable Windows builder artifact that reconstructs/builds the real EXE locally from user-owned game data, and separately preserve the synthetic CI scaffold with an explicit non-playable notice.
[x] Add and validate Linux-hosted Windows cross-build support with LLVM-MinGW; download the offline build kit into the model environment and reproduce a valid Windows PE locally without Visual Studio.

[x] **Make offline source bundles self-contained** — PR #8 / Actions `36096273445`: full-history checkout, `git bundle --all`, smoke-clone verification on Windows and Linux; **144/144** unit tests green.
