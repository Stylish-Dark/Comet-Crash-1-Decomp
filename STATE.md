# STATE

## Current objective

Recover **Comet Crash (NPEB00142 v1.00)** into readable game-domain C/C++ and rebuild it as a normal native Windows game. Static recompilation is retained only as a reverse-engineering/runtime oracle.

## Current phase

**Native source recovery has begun.**

The prior static-recomp path successfully proved that the title can be lifted into an x86-64 Windows process and drove D3D12 far enough to present frames and issue real draw calls. Its continuing PS3-runtime/synchronisation burden made clear that it should not be the final port architecture.

## Exact-title baseline retained

- 3,409 original PPU functions from OPD analysis.
- 171 firmware imports across 18 libraries.
- Two embedded SPU ELFs.
- Reference rebuilt ELF SHA-256: `3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
- Existing PPU/SPU lifts and Boot Fix logs remain valid reverse-engineering evidence.

## New recovery work completed

- Added `decomp/` as the primary source-recovery tree and documented rules that distinguish semantic decompilation from register-level lifting.
- Added `tools/decomp_source_refs.py`, which follows TOC-relative PPU loads to printable strings and maps each reference back to the owning function from `EBOOT.functions.json`.
- On the exact title, the tool recovers 1,257 printable references across 271 functions.
- Identified PPU function `0x000E5A34..0x000E65D4` as originating in `arenaGraphics.cpp`, with seven exact surviving line anchors: 1034, 1081, 1100, 1115, 1130, 1143 and 1188.
- Identified `0x000ECCA8` as a major gameplay/arena asset-bootstrap candidate through references to player ship, resource, agent, platform, weapon, structure, mine, bullet, gateway and font assets.
- Semantically decompiled the first game-specific function, `0x000D5D5C..0x000D5E18`. It builds `"%slevel%u.map"`, reads the current level index at root-state offset `0x2D451C`, passes the path/index/mode into `0x000D91B0` using subobject offset `0x2D6438`, and returns the callee result as a boolean. Transitional native C++ is in `decomp/recovered/level_map_000D5D5C.cpp`.

## Immediate target

Recover the level/map subsystem outward from `0x000D5D5C`:

1. decompile `0x000D91B0` enough to identify the map object, input mode and file format path;
2. map all callers of `0x000D5D5C` and stores/reads around root offsets `0x2D451C` and `0x2D6438`;
3. replace the raw offsets with provisional typed structures once field widths/lifetimes are proven;
4. in parallel, segment `0x000E5A34` using its `arenaGraphics.cpp` source-line anchors so the native renderer boundary can be designed from game semantics rather than RSX emulation.

## Legacy static-recomp track

The previous `port/`, compatibility patches, build pipeline and boot diagnostics are intentionally retained. They are now an oracle for:
- function/control-flow verification;
- dynamic call/asset traces;
- original state transitions;
- differential testing while native recovered subsystems come online.

Do not spend new work making the compatibility runtime more emulator-like unless it directly answers a decompilation question.

## Important files

- `decomp/README.md` — source-recovery rules and architecture boundary.
- `decomp/recovered/` — semantic recovered C/C++.
- `docs/decomp/source-anchors.md` — proven address/source concept anchors.
- `tools/decomp_source_refs.py` — TOC/string-to-function recovery tool.
- `WORK_QUEUE.md` — bounded next reverse-engineering/decompilation units.
- `PROJECT_PLAN.md` — new native-source architecture and phases.
- `DECISIONS.md` — settled direction and constraints.
- `SESSION_LOG.md` — chronological work history, including the old static-recomp phase.
