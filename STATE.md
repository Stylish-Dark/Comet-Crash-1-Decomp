# STATE

## Current objective

Recover **Comet Crash (NPEB00142 v1.00)** into readable game-domain C/C++ and rebuild it as a normal native Windows game. Static recompilation is retained only as a reverse-engineering/runtime oracle.

## Current phase

**Native source recovery now includes an initial-state arena renderer. All 29 built-in maps construct native terrain/structure state. Themed environment slots 34/37/38, player starts, event heaps, shuffled RNG, pending-command/retry boundaries and construction validation are recovered. Original maps from all four environment themes render with original assets. This remains an arena preview: construction commits, movement, combat, HUD and progression are not wired into a playable loop.**

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

**Arena render-target setup and the arena model bootstrap are now represented as native data. Current work advances into the model-object initializer at `0x00105308` to type the recovered 0x90-byte model slots.**

Recovered from `arenaGraphics.cpp`:

- seven source-line anchors divide framebuffer-complete checkpoints at lines 1034, 1081, 1100, 1115, 1130, 1143 and 1188;
- texture bind/parameter/image allocation, framebuffer bind/attachment and completeness wrappers are identified by exact GL constants and handle use;
- texture and framebuffer handle slots in root state `+0x2D44xx` are mapped;
- display-scaled color/depth, three RGB16F targets, a 384x384 target, 80x64 targets and the three-FBO MRT loop are expressed in a native `ArenaRenderTargetPlan`;
- the renderer mode's exact 2x/1x scaling policy is recovered while PSGL-specific parameter `0x6022` remains provenance-only;
- a standalone CMake/CTest gate now compiles the recovered native C++ tree instead of allowing it to exist as unchecked pseudocode.

Asset/bootstrap recovery now adds:

- `0x000ECCA8..0x000EE87C` contains exactly 37 direct model-initializer calls to `0x00105308`;
- every model destination lies on a strict 0x90-byte table beginning at root offset `0x2D2DC0`;
- touched slots are 0..33, 35, 36 and 39; slots 34, 37 and 38 remain intentionally unaccounted for;
- exact path, slot/root offset, option word and both floating arguments are preserved in a native 37-entry `ArenaModelAssetSpec` manifest;
- fonts, font shaders, `shaders.bin`, particle textures and `gui_quad_shader` are pinned as the non-model portions of the same bootstrap.

Model geometry recovery now adds:

- `+0x04` = unique vertex count;
- `+0x08` = submesh count;
- `+0x0C` = final 16-bit index count;
- `+0x10` / `+0x14` = full / compact CPU vertex-array pointers;
- `+0x18` = pointer to 0x78-byte submesh records;
- `+0x1C` = 16-bit index-array pointer;
- `+0x20` / `+0x24` = GPU array-buffer handles for the two vertex layouts;
- full vertices are 0x20 bytes: position.xyz, normal.xyz, texcoord.xy;
- compact vertices are 0x14 bytes: position.xyz, texcoord.xy;
- the temporary OBJ face-reference vector uses 0x0C-byte records and collapses into the unique-vertex/index buffers.

A correction from the previous pass is now pinned in source and documentation: helper `0x00105088..0x00105307` receives the **material subobject at submesh+0x10**, not the root model. Its fields are material-relative `+0x38/+0x3C/+0x40/+0x48`, corresponding to submesh-relative `+0x48/+0x4C/+0x50/+0x58`. The shader decision tree itself was correct and is now exposed as `choose_default_material_shader()`.

Renderer-side indexed drawing proves submesh `+0x00/+0x04/+0x08/+0x0C` as first index, index count, minimum vertex and maximum vertex respectively; first index is multiplied by two for the u16 index-buffer byte offset. MTL parsing now also proves material `+0x38/+0x3C/+0x40/+0x44` as `map_Kd` diffuse, `map_Ks` specular, `bump`, and `cube` texture resources. The first three use one 2D loader path; `cube` uses a distinct cube-texture loader. MTL scalar/color parsing is now also typed: `Ka` writes ambient RGB to material `+0x00..+0x08`, `Kd` diffuse RGB to `+0x10..+0x18`, `Ks` specular RGB to `+0x20..+0x28`, and `Ns` writes `Ns * 0.12800000607967377` to `+0x30`. `+0x0C/+0x1C/+0x2C` are now recovered as the zero-initialized W lanes of the ambient/diffuse/specular vec4 shader inputs, and material `+0x34` is `useTeamColor`, set by the `newmtl team*` prefix policy. Both floating model-loader arguments are now recovered: `f1` is the literal geometry scale applied to OBJ vertex x/y/z components, while `f2` is a signed bounding-radius scale. Model `+0x2C` accumulates `max(abs(f2) * length(scaled_vertex))` and is negated at finalization when `f2 < 0`. Arena manifest fields are now named `geometry_scale` and `signed_radius_scale`. Model `+0x28` is now recovered as a vertex Y offset: in the literal OBJ `v` parser, legacy option bit `0x20` adds this field to the second (Y) position component after geometry scaling and before bounding-radius accumulation. The root-model geometry/load-parameter block from `+0x04` through `+0x2C` is therefore semantically named. The alternate submesh path is now also structurally recovered.  Submesh
`+0x64` gates transient expanded batching; `+0x68` is a per-frame batch
instance count; `+0x6C` points at an expanded 32-byte vertex stream;
`+0x70` points at a u32 index stream; and `+0x74` points at one 16-byte
`objInfo` vec4 per batched object.  The original vertex programs independently
retain the parameter names `position_tx`, `normal_ty`, and `objInfo`.
Writer `0x000FEFB0` allocates exactly `N*V*32`, `N*I*4`, and `N*16`
bytes for those streams, renderer `0x00100750` issues one multiplied u32-index
draw, and `0x000FF594` clears the batch counts after the render cycle.

Current target: recover the producer-side transform behind the 256-byte work
descriptors built through `0x00138248`, prove or disprove that the small active
embedded SPU is the batch producer, and finish exact OBJ source-index
normalization/deduplication semantics.

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
- `decomp/include/comet/model_geometry.hpp`
- `decomp/include/comet/model_load_parameters.hpp`
- `decomp/include/comet/submesh_batch.hpp`
- `docs/decomp/submesh-expanded-batch.md`
- `docs/decomp/model-y-offset.md`
- `decomp/include/comet/material_shader_policy.hpp`
- `decomp/include/comet/material_textures.hpp`
- `decomp/include/comet/material_properties.hpp`
- `docs/decomp/model-object.md`
- `WORK_QUEUE.md`
- `PROJECT_PLAN.md`
- `DECISIONS.md`
- `SESSION_LOG.md`

## Native geometry implementation note (October 2026)

The development branch `decomp/obj-face-index-foundation-20261008` / PR #31 contains port-facing OBJ face-reference and `f` directive parsers, stable triplet deduplication, convex fan triangulation, transactional incremental submesh assembly, u16 index-limit enforcement, and CMake/CTest coverage. This is **implementation progress**, not new machine-code evidence. Original source-index normalization, triangulation, and expanded-batch producer semantics remain unproven. Do not promote these into recovered legacy facts without disassembly or dynamic-oracle evidence.

## Native graphics milestone (2026-10-08)

Branch `decomp/native-arena-renderer-20261009` continues PR #31. See `docs/decomp/native-viewer.md` for build/controls and boundaries. Native OBJ/MTL assembly, confined asset loading, DDS top-mip decoding and optional GPU rendering are implemented. Real-title verification: 37 models, 20,802 vertices, 7,506 triangles; rock-comet 625 vertices/1,152 triangles; level 0 extent 16/147 primary/1 secondary. All 11 render textures and 10 FBOs pass completeness checks under Mesa llvmpipe. No original binaries or assets are published.

The exact ELF hash was rechecked. `docs/decomp/model-loader-runtime-evidence.md` records further instruction evidence for `.dds` rewriting and the full-triplet triangle/quad branch. Native negative indices, generated normals, general polygons, basename adaptation and viewer controls remain port policies. Windows packaging is provided by `native-source-port` CI; do not claim Windows execution until that job passes.

## Native arena continuation (2026-10-09)

- New `arena_state` consumes supported primary/secondary records from `0xD7A48`, transactionally creates grid/ownership/model state and preserves orientation words.
- `0xF1030` / `0xF0B80` / `0x198760`: strict due-event gating, pending/retry orchestration, exact shuffled RNG and timed repeats. Resolver requires recovered exclusion masks and ordered entry flags, rather than guessed occupancy.
- `arena_construction`: recovered cost/eligibility/duplicate boundary, bounded FIFO and scalar debit after supplied constructor succeeds. Full constructor/update semantics still open.
- `--arena` renders map instances with original themed assets and background transforms. Composite parts and original rendering effects remain open.
- Evidence and explicit native policies: `docs/decomp/arena-state.md`.
- Next: recover original construction-ring consumer and complete entity constructor/stage transitions, then unit movement/update dispatch.

## SPU gameplay bottleneck recovered

Six raw SPURS job binaries were missing from the embedded-ELF inventory. Their pointer pairs and hashes are now recorded in `docs/decomp/arena-spu-jobs.json`; `tools/extract_arena_spu_jobs.py` recovers them from the user-owned ELF. Construction ring orchestrator CAB90 submits the FDE54 job (10,880 bytes at 0x1C5580), not a PPU D6088 commit call. Next: replace that SPU command consumer's state writes with native construction; trace the larger cell/entity jobs for movement/combat. The two embedded ELF count remains correct for ELF images, but is not the full SPU code inventory.


## 2026-10-09: native SPU construction grid transaction

Recovered the FDE54 raw job route field and transactional grid placement in `decomp/src/arena_routes.cpp`; see `docs/decomp/arena-routes.md`. Forty original-routine oracle fixtures matched all 23,040 route bytes. Sixty full original-job fixtures matched status and committed 9,344-byte snapshots (26 accepted, 34 rejected) for ordinary placement, gates and visibility removal. Pair-route storage is compact 12-slot indexing; path lengths use a distinct 4x4 matrix. Opcode 0 atomic cell-list removal, opcode 29 base relocation, entity/resource completion integration and simulation jobs remain unfinished. PR33 prior commit 08fcd2f passed both GitHub tests and native-source-port workflows. This remains an incomplete PC port.


Base relocation grid branch recovered and verified against 24 full original-job fixtures (9 accepted, 15 rejected). PPU completion `0xCCC70..0xCCD3C` invokes D6088 with commit=1 after successful SPU validation; constructor failure queues opcode 255 for release. Therefore completion integration must run the recovered constructor boundary after grid reservation, and persist relocated-base events/coordinates separately.


Opcode 0 cell-list clear is now native: original ordering drains the list before route validation and preserves that drained state even on grid rejection. Twenty-four full original-job fixtures matched status/grid output (16 accepted, 8 rejected), and the private atomic transport checked the original list count was cleared on every outcome. Concurrent reservation retries and broader list/player fixtures remain transport validation work. All published checks at 7d8ae68 passed.


Broader routing comparison passed 36 original-job fixtures varying one to four players, team assignments, owner and inactive-player flags (30 accepted, 6 rejected). Together with placement/base/clear fixtures, 144 full-job comparisons match original status and committed grid snapshots. All 13 native checks pass after the shared route rebuild/refactor; portable regression coverage preserves inactive/same-team pair bytes and lengths.

## 2026-10-09: entity construction continuation

Native recovery now includes shared entity core FECFC, upgrade-core transitions, complete opcode-21 entity-slot initialization, and the funded reservation/completion/release boundary. The initializer matched 1,000 decoded-original fixtures and opcode 21 matched 500. Allocation/cell-list insertion, other constructor types, insufficient-funds notifications, completion retries and simulation remain incomplete. See `docs/decomp/arena-entity-construction.md`; these boundaries are not wired into the viewer gameplay loop.

## 2026-10-09 continued: entity allocation and ordered cell banks

Recovered CA514 insertion, single-index FIFO allocation, both 256-byte entity-bank copies, 1117F0 packed cell records, selected-bank target lookup and summary-counter updates. The first packing call may change coordinates before the second; both list pointers retain their original cell. A thousand original-instruction packer fixtures matched all entry bytes, coordinates and result. Construction integration now calls real opcode-21 initialization and insertion. See `docs/decomp/arena-entity-storage.md`. Prior commit 7e511e6 passed both GitHub workflows. Remaining: pool bootstrap/recycling, other constructors, simulation readiness/movement/combat, unfunded effects/retries and interactive gameplay.
