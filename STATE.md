# STATE

## Current objective

Recover **Comet Crash (NPEB00142 v1.00)** into readable game-domain C/C++ and rebuild it as a normal native Windows game. Static recompilation is retained only as a reverse-engineering/runtime oracle.

## Current phase

**Native source recovery is underway. Level-map loading, arena render-target setup, arena model bootstrap, and the core model geometry layout are now represented natively. Current work is inside the OBJ/MTL model loader and 0x78-byte submesh/material records.**

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

Renderer-side indexed drawing proves submesh `+0x00/+0x04/+0x08/+0x0C` as first index, index count, minimum vertex and maximum vertex respectively; first index is multiplied by two for the u16 index-buffer byte offset. MTL parsing now also proves material `+0x38/+0x3C/+0x40/+0x44` as `map_Kd` diffuse, `map_Ks` specular, `bump`, and `cube` texture resources. The first three use one 2D loader path; `cube` uses a distinct cube-texture loader. MTL scalar/color parsing is now also typed: `Ka` writes ambient RGB to material `+0x00..+0x08`, `Kd` diffuse RGB to `+0x10..+0x18`, `Ks` specular RGB to `+0x20..+0x28`, and `Ns` writes `Ns * 0.12800000607967377` to `+0x30`. Unknown gaps `+0x0C/+0x1C/+0x2C/+0x34` remain intentionally unnamed. Both floating model-loader arguments are now recovered: `f1` is the literal geometry scale applied to OBJ vertex x/y/z components, while `f2` is a signed bounding-radius scale. Model `+0x2C` accumulates `max(abs(f2) * length(scaled_vertex))` and is negated at finalization when `f2 < 0`. Arena manifest fields are now named `geometry_scale` and `signed_radius_scale`. Model `+0x28` is now recovered as a vertex Y offset: in the literal OBJ `v` parser, legacy option bit `0x20` adds this field to the second (Y) position component after geometry scaling and before bounding-radius accumulation. The root-model geometry/load-parameter block from `+0x04` through `+0x2C` is therefore semantically named. Next: recover the alternate submesh batching path at `+0x68..+0x74`.

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
- `docs/decomp/model-y-offset.md`
- `decomp/include/comet/material_shader_policy.hpp`
- `decomp/include/comet/material_textures.hpp`
- `decomp/include/comet/material_properties.hpp`
- `docs/decomp/model-object.md`
- `WORK_QUEUE.md`
- `PROJECT_PLAN.md`
- `DECISIONS.md`
- `SESSION_LOG.md`
