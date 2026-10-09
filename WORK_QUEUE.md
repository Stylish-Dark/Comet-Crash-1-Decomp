# Active gameplay continuation

- FDE54 grid transaction and funded completion boundary are recovered. Recover entity allocation/cell insertion at CA514, remaining constructors, insufficient-funds notifications and completion retries.
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


## 2026-10-09: native SPU construction grid transaction

Recovered the FDE54 raw job route field and transactional grid placement in `decomp/src/arena_routes.cpp`; see `docs/decomp/arena-routes.md`. Forty original-routine oracle fixtures matched all 23,040 route bytes. Sixty full original-job fixtures matched status and committed 9,344-byte snapshots (26 accepted, 34 rejected) for ordinary placement, gates and visibility removal. Pair-route storage is compact 12-slot indexing; path lengths use a distinct 4x4 matrix. Opcode 0 atomic cell-list removal, opcode 29 base relocation, entity/resource completion integration and simulation jobs remain unfinished. PR33 prior commit 08fcd2f passed both GitHub tests and native-source-port workflows. This remains an incomplete PC port.


Base relocation grid branch recovered and verified against 24 full original-job fixtures (9 accepted, 15 rejected). PPU completion `0xCCC70..0xCCD3C` invokes D6088 with commit=1 after successful SPU validation; constructor failure queues opcode 255 for release. Therefore completion integration must run the recovered constructor boundary after grid reservation, and persist relocated-base events/coordinates separately.


Opcode 0 cell-list clear is now native: original ordering drains the list before route validation and preserves that drained state even on grid rejection. Twenty-four full original-job fixtures matched status/grid output (16 accepted, 8 rejected), and the private atomic transport checked the original list count was cleared on every outcome. Concurrent reservation retries and broader list/player fixtures remain transport validation work. All published checks at 7d8ae68 passed.


Broader routing comparison passed 36 original-job fixtures varying one to four players, team assignments, owner and inactive-player flags (30 accepted, 6 rejected). Together with placement/base/clear fixtures, 144 full-job comparisons match original status and committed grid snapshots. All 13 native checks pass after the shared route rebuild/refactor; portable regression coverage preserves inactive/same-team pair bytes and lengths.

## 2026-10-09: entity construction continuation

Recovered shared entity initializer FECFC, upgrade-core transitions 12869C/128C9C, the complete opcode-21 slot constructor and funded reservation/completion/release integration. Decoded-original checks matched 1,000 core fixtures and 500 opcode-21 fixtures. Base completion preserves the event heap. See `docs/decomp/arena-entity-construction.md`. Next: CA514 allocation and ordered cell-list insertion, remaining constructors, simulation bank membership and FE5D8 movement/combat. No playable loop yet.

## 2026-10-09 continued: entity allocation and ordered cell banks

Recovered CA514 insertion, single-index FIFO allocation, both 256-byte entity-bank copies, 1117F0 packed cell records, selected-bank target lookup and summary-counter updates. The first packing call may change coordinates before the second; both list pointers retain their original cell. A thousand original-instruction packer fixtures matched all entry bytes, coordinates and result. Construction integration now calls real opcode-21 initialization and insertion. See `docs/decomp/arena-entity-storage.md`. Prior commit 7e511e6 passed both GitHub workflows. Remaining: pool bootstrap/recycling, other constructors, simulation readiness/movement/combat, unfunded effects/retries and interactive gameplay.

### Entity index-pool bootstrap

CAF20 initializes root+AB00 from capacity root+2D44F0: ascending indices, head zero and full availability. Added native reset_arena_entity_index_pool and used it in the construction/insertion integration test. Invalid native capacity preserves the pool. Other world-reset fields, entity-bank memory allocation and index recycling remain separate. CPU 16/16 and graphics 17/17 checks pass after this addition.

### Opcode-20 constructor

Recovered complete D7090..D7134 slot initialization with mutable-table tuning inputs. Five hundred original-instruction fixtures matched all 256 bytes. Entity fixture total is now 3,000. Commit a5f4550 passed Python, Linux graphics and Windows native CI. The remaining constructor types, other world bootstrap/reset fields, recycling and simulation/gameplay remain unfinished.
