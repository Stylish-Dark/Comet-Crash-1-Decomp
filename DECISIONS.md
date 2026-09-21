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

With 61/61 local tests green and the lift/HLE gates clean, the next engineering decisions must be driven by a real Windows native boot log. Do not invent the next blocker in advance.

## D011 — GitHub/repository continuity is canonical

The repository must always carry enough structured state for a fresh conversation to resume without chat history. `CONTINUITY.md` is read first; meaningful milestones update `NEXT.md` and `SESSION_LOG.md` immediately.
\n## D012 — Recovered advanced tree supersedes the temporary scaffold\n\nThe SHA-verified recovered 68-file source tree restored at `151ede4` is canonical. Do not reintroduce the earlier M1-only scaffold or treat it as a newer project state.\n