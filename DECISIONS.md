# DECISIONS

## D001 — Static recompilation is the implementation path

Use `ps3recomp` as the native-port foundation. RPCS3 is a behavioral/reference baseline, not the shipped runtime.

## D002 — Preserve vanilla behavior before PC refinements

Controller-driven original behavior comes first. Native absolute mouse-pointer injection, high-refresh changes and other timing-sensitive PC improvements wait until missions are demonstrably stable.

## D003 — Never redistribute proprietary title material

No PKG/RAP/RIF, EBOOT/ELF, extracted game assets, Sony SDK material, or embedded proprietary SPU binaries are committed. Original port code, patches, metadata and derived non-infringing analysis are allowed.

## D004 — Do not fake core gameplay

Missing guest behavior is repaired through faithful runtime/HLE/lift work. Approximate host-side substitutions are not acceptable for core game simulation merely to obtain a boot.

## D005 — Optional PS3 services must not return fabricated success

Online/media/export/trophy-adjacent services that cannot be meaningfully supported on PC must fail explicitly or use an intentional offline-safe behavior. Returning success without delivering the required callback/state is rejected because it can deadlock the title later.

## D006 — SPU work is evidence-driven

Lift/dispatch SPU programs according to actual title use. The large SPU is established as MultiStream MP3 middleware; the small active SPU remains unidentified.

## D007 — Pin ps3recomp and fail loudly on upstream drift

Current pin: `d3ed1a5c946a9c5370b51631e13371a1adf70396`.

The Comet-specific runtime patchers encode verified expectations about this upstream tree. A patch mismatch is a stop condition requiring review, not something to paper over.

## D008 — HLE coverage is a build gate

The native build must not proceed while any of Comet Crash's 171 imports lack either a pinned-runtime handler or an explicit port override.

## D009 — Compatibility-mode mouse first

Current mouse implementation injects onto the existing PS3 pad abstraction while preserving physical controller input. Direct absolute world/UI pointer mapping remains a later improvement after native gameplay is proven.

## D010 — First-boot evidence outranks further speculative static work

With the current regression gates green and the lift/HLE gates clean, the next engineering decisions must be driven by a real Windows native boot log. Do not invent the next blocker in advance.

## D011 — GitHub/repository continuity is canonical

The repository must always carry enough structured state for a fresh conversation to resume without chat history. `STATE.md` is read first; meaningful milestones update `STATE.md`, `WORK_QUEUE.md`, and `SESSION_LOG.md` immediately.

## D014 — The toolchain pin is an execution gate, not documentation

Analysis, lift and build commands must verify that the ps3recomp checkout is a Git checkout at the exact commit in `config/ps3recomp.lock`. A mismatched or unverifiable checkout is rejected before generated code or runtime patches are used.

## D015 — Actionable PPU no-op fallbacks are build blockers

Generated PPU code must contain no actionable lifter `TODO:` instruction holes and no `unsupported SPR -- no-op` fallbacks after Comet compatibility patching. Silent unsupported guest instructions are not accepted as successful lifts. Deterministic data-like `.word` fallbacks are governed separately by D026 and are accepted only when their exact baseline matches.

## D016 — ps3recomp cache is disposable and reproducible

`external/ps3recomp` is a pinned toolchain cache, not a user workspace. Existing checkouts are restored to the exact locked upstream tree before each pipeline run, then Comet-specific runtime patches are reapplied deterministically. Local edits inside that cache are not preserved.

## D017 — Runtime evidence is provenance-bound and conservative

Every native boot artifact should identify the build that produced it and preserve both chronological symptoms and the first subsystem-specific signal. Generic crashes/watchdogs are evidence that execution failed or stalled, not evidence for a particular subsystem. Build provenance, structured boot outcomes and conservative triage are part of the debugging contract.

## D018 — Generated-source audits are byte-safe

Generated ps3recomp source is not assumed to be UTF-8 because upstream tooling may write using the host default code page. Audits whose signatures are ASCII-only operate on a reversible one-byte decoding so Windows code-page punctuation cannot turn a valid generated lift into an audit crash.

## D019 — Native execution requires exact build provenance

A normal `run` must reject an EXE that is not cryptographically bound to the current tracked source snapshot, exact ps3recomp pin and complete HLE baseline. A diagnostic override may exist, but its use must be explicit and preserved in the boot evidence.

## D020 — Boot evidence must be self-verifying

The structured boot summary is not trusted merely because it exists. It binds to the finalized text log by SHA-256 and is independently re-derived by `tools/verify_boot_bundle.py`. Runtime debugging should not proceed from a bundle that fails this integrity/provenance check.

## D021 — Graphics initialization failures must retain the first exact API error

Pinned D3D12 setup failures are logged at the failing API site with HRESULT/Win32 detail where available. The later aggregate “D3D12 init FAILED” line is not allowed to replace a more specific earlier graphics signal during triage.

## D022 — Windows guest PPU thread exit uses CRT thread termination, not longjmp

Pinned ps3recomp creates Windows guest PPU host threads with `_beginthreadex()`. Real Comet Crash boot evidence showed that using `longjmp()` to escape a deep recompiled guest call chain during `sys_ppu_thread_exit` can raise `STATUS_BAD_FUNCTION_TABLE (0xC00000FF)` in the Windows unwinder. After the syscall has stored exit status and signalled joiners, Windows exits that matching CRT thread with `_endthreadex(0)`. POSIX retains the existing `longjmp()` path.

## D023 — Allocator abort bypass is diagnostic only

The observed Boot Fix 2 guest abort at return address `0x001A4E90` is not treated as permission to suppress allocator failures generally. Boot Fix 3 replaces only the evidenced branch-and-link at `0x001A4E8C` with a NOP so the next run can distinguish an isolated bad-free symptom from broader memory corruption. This patch is not accepted as permanent gameplay behaviour; recurring allocator corruption requires fixing the underlying runtime/memory defect.

## D024 — Do not advance past repeated allocator assertions; instrument the first invariant failure

Boot Fix 3 demonstrated that suppressing the first allocator abort merely carries corrupted heap state forward into another Dinkumware `mspace_free` assertion. Further abort-site NOPs are prohibited as a debugging strategy. Restore the original abort and capture the first failing free's caller, pointer, chunk metadata and mspace state; fix the invalid free or earlier corrupting write instead.

## D025 — Offline source artifacts must be self-contained and smoke-cloned

A source bundle is not considered a valid continuity artifact merely because `git bundle create` succeeds. Artifact-producing CI uses a full-history checkout, creates bundles from all relevant refs, and performs a fresh clone/HEAD verification before upload. This prevents model-side builds from depending on omitted shallow-history parents.


## D026 — Data-like PPU raw-word fallbacks require an exact baseline

The pinned reference lift contains 3,936 `.word 0x........` TODO fallbacks even after all actionable Comet instruction holes are resolved. These are not silently ignored: the pipeline pins both the exact count and the SHA-256 of the ordered raw-word value sequence. Any changed count/digest is a hard failure, and any non-`.word` TODO or unsupported-SPR no-op remains independently fatal.

## D027 — Recovered PPU jump tables are a deterministic lift gate

A successful PPU lift must reproduce the exact reference computed-switch structure, not merely contain zero unsupported mnemonic TODOs. The reference title currently requires 99 recovered jump-table dispatchers, 694 case occurrences, 689 unique targets, and ordered per-dispatcher SHA-256 `ea920e593b23773631066e58b546c23f71007d145b9d22ffac4a75ea92b1e7b7`. Any count, grouping, target-order or digest drift is a hard failure. This prevents interior basic-block cases such as `0x00052DF4` from silently falling back to the global function-entry dispatcher.

## D028 — LLVM-MinGW user builds statically link the LLVM C++ runtime

Linux-hosted Windows builds must not require separately shipped `libc++.dll` or `libunwind.dll`. The MinGW path links the LLVM runtime statically while retaining normal Windows system/D3D/XInput DLL dependencies. CI inspects the produced PE import table and fails if either LLVM runtime DLL reappears. The native MSVC/clang-cl path is unchanged.

## D029 — Inline PPU jump-table recovery is verified against the source ELF

The generated computed-switch count/digest gate is necessary but not sufficient by itself. The lift pipeline independently scans the supported PPC64 ELF for the narrow signed-relative `lwzx/extsw/add/mtctr/bctr` inline-table structure, decodes its targets, and requires each structural target set to exist in generated C++. Any structural inline table missing from the lift is a hard failure. This catches the class of defect that produced the unresolved interior target `0x00052DF4` even if aggregate generated-switch statistics look plausible.

## D030 — Unresolved aligned title-text dispatch is fatal

After normal function lookup, OPD repair and the existing invalid-vcall handling, an unresolved 4-byte-aligned target inside the title's executable range must terminate the run rather than return to lifted guest code. Boot Fix 6 demonstrated why: the missed `0x00052DF4` in-function switch target made `func_0005207C` return before its epilogue, leaving guest `r1` 0x210 bytes low and callee-saved state unrestored. Continuing from that state converts a control-flow defect into misleading downstream memory corruption. Fail fast and preserve diagnostics instead.

## 2026-09-26 — Pivot final architecture from static recompilation to semantic decompilation/native rewrite

Decision: the shipping port will not be the mechanically lifted PPU program running on an increasingly complete PS3 compatibility runtime. The repository will recover game-domain logic into readable C/C++ and replace PS3-facing subsystem boundaries with native PC implementations.

Rationale: the static-recomp track proved invaluable for function discovery, exact control flow and runtime evidence, but runtime progress increasingly depended on reproducing PS3 scheduler/GCM/SPURS/HLE semantics. That is useful as an oracle and poor as the desired final architecture. The recovered-source track keeps the knowledge already gained while moving complexity into explicit game systems that can be understood, tested and maintained.

Consequence: `ps3recomp`, Boot Fix packages and runtime diagnostics remain valid reverse-engineering tools. New work should decompile and type game systems first; compatibility-runtime work is justified only when it yields evidence needed by the native rewrite.

