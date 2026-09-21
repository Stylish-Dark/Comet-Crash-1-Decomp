# DECISIONS

## D001 — Static recompilation is the implementation path
Use `ps3recomp` as the native-port foundation. RPCS3 is a behavioral/reference baseline, not the shipped runtime.

## D002 — Preserve vanilla behavior before PC refinements
Controller-driven original behavior comes first. Native absolute mouse-pointer injection, high-refresh changes and timing-sensitive PC improvements wait until missions are demonstrably stable.

## D003 — Never redistribute proprietary title material
No PKG/RAP/RIF, EBOOT/ELF, extracted game assets, Sony SDK material, or embedded proprietary SPU binaries are committed.

## D004 — Do not fake core gameplay
Missing guest behavior is repaired through faithful runtime/HLE/lift work.

## D005 — Optional PS3 services must not return fabricated success
Unsupported online/media/export services fail explicitly or use intentional offline-safe behavior rather than returning success without required callbacks/state.

## D006 — SPU work is evidence-driven
The large SPU is established as MultiStream MP3 middleware; the small active SPU remains unidentified.

## D007 — Pin ps3recomp and fail loudly on upstream drift
Current pin: `d3ed1a5c946a9c5370b51631e13371a1adf70396`.

## D008 — HLE coverage is a build gate
All 171 Comet imports must be covered by the pinned runtime or explicit port overrides.

## D009 — Compatibility-mode mouse first
Current mouse injection uses the original pad abstraction while preserving physical controllers. Direct absolute pointer injection comes after native gameplay stability.

## D010 — First-boot evidence outranks speculative static work
With 61/61 recovered tests green and lift/HLE gates clean, the next engineering decisions must be driven by a real Windows native boot log.

## D011 — Repository continuity is canonical
A fresh session reads `CONTINUITY.md` first. Significant work updates `NEXT.md` and `SESSION_LOG.md`.

## D012 — Recovered advanced checkpoint supersedes the temporary scaffold
The recovered `3250838` history and follow-up `c0089d4` state are more authoritative than the earlier temporary GitHub reconstruction. Stale remote scaffold assumptions must not override verified recovered state.
