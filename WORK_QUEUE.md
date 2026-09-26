# WORK QUEUE

The canonical direction is now semantic decompilation/native rewrite. Keep work units bounded and evidence-driven.

## Current

[ ] **Recover the level-map loader around `0x000D91B0`**
- Start from the already recovered wrapper `0x000D5D5C`.
- Build the direct caller/callee set for `0x000D91B0`.
- Recover file reads, header/record structure and ownership semantics far enough to name its parameters and return value.
- Track accesses to root offsets `0x2D451C` and `0x2D6438` across neighboring functions.
- Promote `level_map_000D5D5C.cpp` away from raw offsets when the structure layout is supported by multiple references.
- Success: a typed native level/map interface with at least one map-load path expressed without PowerPC register semantics.

## Next

[ ] **Segment and decompile the `arenaGraphics.cpp` function at `0x000E5A34`**
- Use the seven source-line anchors (1034..1188) to divide the large function into semantic blocks.
- Identify direct callees and graphics-state effects for each block.
- Separate game renderer state from PSGL/GCM calls.
- Success: a readable renderer-facing function with enough semantics to define the future native render API.

[ ] **Recover the arena asset bootstrap at `0x000ECCA8`**
- Group asset loads by destination field/registry.
- Identify model/font/shader manager interfaces.
- Replace raw asset-registration sequences with semantic native structures.

[ ] **Build the root game-state type map**
- Collect recurring offsets from game-domain functions.
- Record width, read/write sites, lifetime and likely subobject boundaries.
- Name fields only after cross-reference support.

## Later

[ ] Recover gameplay update loop and entity hierarchy.
[ ] Recover towers/weapons/resources/enemy/wave logic.
[ ] Recover menu/HUD and input abstraction.
[ ] Implement native renderer behind recovered game semantics.
[ ] Implement native audio and save/settings layers.
[ ] Differential-test missions against the original title/oracle.

## Legacy work policy

The static-recomp runner remains available for tracing. Do not treat fixing its PS3 compatibility layer as progress toward the shipping architecture unless the run is needed to answer a concrete decompilation question.
