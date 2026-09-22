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

## D015 — Explicit PPU no-op fallbacks are build blockers

Generated PPU code must contain no generic lifter `TODO:` holes and no `unsupported SPR -- no-op` fallbacks after Comet compatibility patching. Silent unsupported guest instructions are not accepted as successful lifts.

## D016 — ps3recomp cache is disposable and reproducible

`external/ps3recomp` is a pinned toolchain cache, not a user workspace. Existing checkouts are restored to the exact locked upstream tree before each pipeline run, then Comet-specific runtime patches are reapplied deterministically. Local edits inside that cache are not preserved.
