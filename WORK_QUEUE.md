# Active gameplay continuation

- Replace the FDE54 construction SPU job (0x1C5580..0x1C8000); CAB90 ring submission and completion are proved. Recover entity allocation/stage writes inside it.
- Recover per-player resource initialization and exclusion-mask state; wire event resolver to ordered native cell lists.
- Recover unit movement, pathfinding, combat, barracks production and win/loss progression.
- Recover composite base/weapon parts and original HUD/input/camera.
- Arena preview, event boundaries and construction eligibility are implemented; none establishes a playable game.

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
- [x] Recover indexed submesh draw range `+0x00..+0x0C`: first index, index count, minimum vertex, maximum vertex.
- [x] Recover alternate transient batch tail `+0x64..+0x74`: enable flag, instance count, packed 32-byte vertex stream, u32 index stream and per-object `objInfo` stream.
- [x] Recover loader f1 as geometry scale and f2/model `+0x2C` as signed bounding-radius scale/result.
- [x] Recover model `+0x28` as the optional OBJ vertex Y offset, enabled by legacy option bit `0x20` after geometry scaling.
- [x] Recover MTL texture construction for `map_Kd`, `map_Ks`, `bump`, and `cube`: material resource slots +0x38/+0x3C/+0x40/+0x44 and separate 2D/cube loader paths.
- [x] Recover MTL scalar/color directives: Ka ambient RGB, Kd diffuse RGB, Ks specular RGB, and Ns scaled specular exponent with exact 0.128000006 multiplier.
- Recover the producer-side transform that fills `position_tx`, `normal_ty`, and `objInfo`; determine whether the small active SPU is the producer.
- Recover remaining material gaps only when an independent use/write site supports a name.
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

## Native OBJ implementation in progress (not legacy-proof)

[x] Establish independently testable OBJ face references, full `f` directives, triangulation, u16 indexing, and submesh ranges. Hash-based vertex identity and rollback guards have native CTest coverage; see PR #31 and `docs/decomp/obj-face-indices.md`.

[ ] Differentially recover the original loader's signed index normalization, missing vt/vn rules, polygon ordering/triangulation, and source-triplet equality from `0x00105308..0x0010B537`. Until proven, do not label the native standard-OBJ behavior an exact decompilation.

[ ] Capture exact-title loader instructions using `tools/decomp_model_disasm.py` on locally owned ELF; resolve source-index normalization and dedup semantics from actual branch/write sites (`docs/decomp/model-loader-evidence.md`).

[x] Build a standalone native bootstrap executable for recovered level maps and arena model/render metadata.
[x] Load native model geometry and initialize graphical rendering; see docs/decomp/native-viewer.md.
[ ] Recover and enter the first arena update loop. The asset viewer does not advance simulation.

## Next shipping boundary

[x] Load all 37 original arena manifest models and MTLs with recovered scales/options.
[x] Decode shipped diffuse DDS formats and render real submeshes through native GPU buffers.
[x] Instantiate and check all 11 recovered textures / 10 framebuffer configurations.
[x] Add Windows build/package workflow and Linux rendered-pixel verification.
[ ] Resolve level-map primary/secondary entity fields from actual construction consumers.
[ ] Trace arena entity placement and the game-domain update entrypoint; replace with typed native state.
[ ] Integrate a complete static arena scene, then prove one mission update path against the original.
[ ] Recover original shader/team/normal/specular behavior; current fixed-function shading is approximate.
