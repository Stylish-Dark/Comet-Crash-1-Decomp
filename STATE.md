# STATE

## Current objective

Recover **Comet Crash (NPEB00142 v1.00)** into readable game-domain C/C++ and rebuild it as a normal native Windows game. Static recompilation is retained only as a reverse-engineering/runtime oracle.

## Current phase

**Native source recovery is underway. The level-map loader is now structurally recovered and implemented natively; current work has moved into `arenaGraphics.cpp`.**

## Exact-title baseline retained

- 3,409 original PPU functions from OPD analysis.
- 171 firmware imports across 18 libraries.
- Two embedded SPU ELFs.
- Reference rebuilt ELF SHA-256: `3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
- Existing PPU/SPU lifts and Boot Fix logs remain valid reverse-engineering evidence.

## Native recovery completed so far

- Established `decomp/` as the primary source-recovery tree.
- Added `tools/decomp_source_refs.py`; exact-title pass recovers 1,257 printable references across 271 functions.
- Proved `0x000E5A34..0x000E65D4` originates in `arenaGraphics.cpp`, with surviving line anchors 1034, 1081, 1100, 1115, 1130, 1143 and 1188.
- Identified `0x000ECCA8` as a major arena/gameplay asset-bootstrap candidate.
- Semantically recovered wrapper `0x000D5D5C`: it loads the root state's current level ID, formats `level%u.map`, and invokes the level-map loader on the root's `level_map_state` subobject.
- Recovered the loader `0x000D91B0..0x000DA254` far enough to replace its parsing semantics with native code:
  - two source modes in the original: IDs 0..28 from a 29-entry embedded table at `0x00232140`, higher IDs from `"rb"` files;
  - exact map layout: `0x88 + primary_count*0x38 + secondary_count*0x18`;
  - type-`0x0B` primary records normalize two extent bytes and derive runtime extent, defaulting to 24 when absent;
  - exact secondary-record selector/opcode filter recovered;
  - all 29 embedded maps validate against the recovered layout.
- Added `tools/extract_builtin_level_maps.py` so the native port can extract the 29 EBOOT-embedded maps from the user's own copy into ordinary `level0.map..level28.map` files.
- Added native level-map types/parser/file loader under `decomp/include/comet/level_map.hpp` and `decomp/src/level_map.cpp`.
- Root-state offsets now supported strongly enough for provisional semantic names:
  - `+0x2D451C` = `current_level_id`;
  - `+0x2D6438` = `level_map_state`;
  - `+0x2D4520` remains a provisional requested/transition level ID.

## Current target

Segment and decompile `arenaGraphics.cpp` function `0x000E5A34..0x000E65D4`.

Current evidence already identifies it as render-target/framebuffer setup rather than general gameplay logic:

- repeated texture binding/parameter/image-allocation calls;
- framebuffer binding/texture-attachment/completeness checks;
- depth, RGBA8 and floating-point render targets;
- a repeated 80x64 target setup loop;
- renderer resource handles clustered at root-state offsets `0x2D44xx`.

The next step is to turn those address/GL facts into a renderer-facing native descriptor/API without preserving PSGL/GCM execution semantics.

## Legacy static-recomp track

The existing `port/`, compatibility patches, build pipeline and boot diagnostics remain an oracle for control flow, dynamic traces, asset access and differential tests. Do not extend the compatibility runtime unless doing so answers a concrete decompilation question.

## Important files

- `decomp/README.md`
- `decomp/include/comet/level_map.hpp`
- `decomp/src/level_map.cpp`
- `docs/decomp/level-map-format.md`
- `docs/decomp/root-state-map.md`
- `docs/decomp/source-anchors.md`
- `tools/decomp_source_refs.py`
- `tools/decomp_level_map.py`
- `tools/extract_builtin_level_maps.py`
- `WORK_QUEUE.md`
- `PROJECT_PLAN.md`
- `DECISIONS.md`
- `SESSION_LOG.md`
