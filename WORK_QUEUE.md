# WORK QUEUE

The canonical direction is semantic decompilation/native rewrite. Keep work units bounded and evidence-driven.

## Completed

[x] **Recover the level-map loader around `0x000D91B0`**
- Recovered both embedded and file-backed source paths.
- Recovered exact 0x88/0x38/0x18 disk structure and exact size equation.
- Recovered type-0x0B extent normalization/default behaviour.
- Recovered the conditional secondary section and exact selector/opcode filter.
- Validated all 29 embedded level blobs.
- Added native parser/types and an extractor that converts the EBOOT-embedded built-in maps into ordinary user-owned `level*.map` files.
- Promoted root offsets `+0x2D451C` and `+0x2D6438` to `current_level_id` and `level_map_state`.

[x] **Segment and decompile `arenaGraphics.cpp` function `0x000E5A34`**
- Use source-line anchors 1034, 1081, 1100, 1115, 1130, 1143 and 1188 as block boundaries.
- Confirm the PSGL/OpenGL wrapper identities for texture, framebuffer and attachment calls.
- Map renderer resource handles at root offsets `0x2D44xx`.
- Recover each render-target descriptor: dimensions, internal format, data format/type, filtering/wrap and framebuffer attachments.
- Define a native renderer-facing target specification that expresses these semantics without PSGL/GCM calls.
- Success: a readable native render-target setup plan tied back to `arenaGraphics.cpp` addresses/source anchors.

## Current

[x] **Recover the arena asset bootstrap at `0x000ECCA8`**
- Group asset loads by destination field/registry.
- Identify model/font/shader manager interfaces.
- Replace raw asset-registration sequences with semantic native structures.
- Result: 37 model calls collapsed into a typed native manifest; exact 0x90 object-table stride, paths, slot offsets, option words and float arguments are preserved.

[ ] **Recover model object initializer `0x00105308`**
- [x] Map top-level geometry fields `+0x04..+0x24`: vertex/submesh/index counts, full/compact CPU arrays, submesh/index pointers, and both GPU vertex-buffer handles.
- [x] Recover the exact 0x20-byte full vertex layout and 0x14-byte compact layout.
- [x] Recover the 0x0C-byte OBJ face-reference staging stride and 0x78-byte submesh stride.
- [x] Correct the shader-helper boundary: `0x00105088` operates on the material subobject at `submesh+0x10`, not the root model.
- [x] Recover material diffuse/specular/bump/preassigned-shader fields and the default-shader option-mask decision tree.
- Recover the remaining submesh draw-range fields at `+0x00..+0x0C` and `+0x68..+0x74`.
- Trace model fields `+0x28/+0x2C` and both floating loader arguments to proven semantics.
- Recover MTL texture construction for `map_Kd`, `map_Ks`, `bump`, and `cube`.
- Success: replace the opaque 0x78-byte submesh/material record and remaining model provenance fields with typed native structures.

[ ] **Build the root game-state type map**
- Continue collecting recurring offsets from game-domain functions.
- Record width, read/write sites, lifetime and subobject boundaries.
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
