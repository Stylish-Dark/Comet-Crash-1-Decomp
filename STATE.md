# STATE

## Current objective

Recover **Comet Crash (NPEB00142 v1.00)** into readable game-domain C/C++ and rebuild it as a normal native Windows game. Static recompilation is retained only as a reverse-engineering/runtime oracle.

## Latest continuation — pilot preview

PR #53 passed Linux graphics, Windows build/package and regression CI. Added
an opt-in F3 continuous pilot preview using the original slot-0 ship model and
a portable implementation of the original input-vector cap at PPU F2DB4..F2E90.
The build cell follows the ship; pause/focus/disconnect stop held input and F2
restores the base cursor. Six-unit speed, heading and bounds are native preview
policy. Original craft dynamics, collisions and gameplay actions remain open;
this is not an original craft simulation. See docs/decomp/native-pilot.md.
Estimate remains **89% toward the first playable single-player mission**: the
preview is more usable, but the original craft-physics milestone is still open.
Continue from the PR above #53 rather than main.

## Prior continuation — 2026-10-19

Continued PR #52 after both workflows passed. Added recovered scalar stick
normalization plus SDL gamepad controls for native skirmish navigation, build,
factory selection, held release, pause and restart. Focus loss/hot-unplug cancel
input and pause; controller rearming requires neutral input. Bindings and repeat
policy are native preview rules; original craft/aircraft and full pad decoder
remain open. See `docs/decomp/native-gamepad.md` for evidence and validation.
Capability estimate: **89% toward a first playable single-player mission** (88%
prior), not full-port/source coverage. Continue the new PR above #52.

## Current phase

**The active native branch includes original-asset rendering, construction,
factory production, held ground deployment, recovered movement/contact/capture,
base impacts and ordinary death/corpse lifecycle. Preview victory/draw now freezes
the viewer simulation; F2 restores its complete launch checkpoint paused.
Automatic converter capture and signed health-effect consumption are now
recovered and integrated. Kind 44 now has its original hop/dynamics and
contact-triggered area health effect; see `docs/decomp/native-ground-slam.md`.
An opt-in kind-22 Tesla now acquires opposing ground targets and drives its
original kind-5 beam health path; see `docs/decomp/native-tesla-combat.md`.
An opt-in kind-21 cannon now tracks ground targets, launches kind-3 kinetic
rounds and applies deferred knockback; see `docs/decomp/native-cannon-combat.md`.
Other weapon firing/projectiles, autonomous enemies,
original craft controls, original mission rules and
campaign progression remain incomplete. Continue from the latest active PR,
not the older main branch.**

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

**Next gameplay target: additional original weapons/projectiles, autonomous
enemy decisions, craft controls or mission progression. Converter activation,
kind-44 impact damage and bounded kind-22 Tesla targeting/beam damage are
integrated. Original win/loss, AI and campaign progression remain unrecovered.**

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

### Entity index-pool bootstrap

CAF20 initializes root+AB00 from capacity root+2D44F0: ascending indices, head zero and full availability. Added native reset_arena_entity_index_pool and used it in the construction/insertion integration test. Invalid native capacity preserves the pool. Other world-reset fields, entity-bank memory allocation and index recycling remain separate. CPU 16/16 and graphics 17/17 checks pass after this addition.

### Opcode-20 constructor

Recovered complete D7090..D7134 slot initialization with mutable-table tuning inputs. Five hundred original-instruction fixtures matched all 256 bytes. Entity fixture total is now 3,000. Commit a5f4550 passed Python, Linux graphics and Windows native CI. The remaining constructor types, other world bootstrap/reset fields, recycling and simulation/gameplay remain unfinished.

## 2026-10-13 continuation: explicit native ground transport

Continued f5a8538 with opt-in transport for seven deployed ground kinds. Original
bound directions/headings and the original FMA position-update arithmetic feed
native x/z travel, both-bank record migration, cached cells and summary counts.
The scene refreshes position/quaternion; M toggles viewer transport and replay
has transport/expect-unit-cell. Extent validation is consistent across simulation
entrypoints. See docs/decomp/native-ground-transport.md.

This is a native preview policy, not an exact port of original steering,
acceleration, collision, crowds or auxiliary unit state. The recovered original
141A0..141EC window matched 4,000 finite numeric output lanes. Capability estimate:
43% toward a playable single-player mission, up from 41%. No combat/mission credit.
Next original recovery boundary is 14060/13998 route steering and 158B8 unit
auxiliary-state update, followed by weapons/base interaction and mission rules.

## 2026-10-13 — original ground steering and base arrival

Recovered complete SPU leaf `13998..1405C` and integrated it with opt-in live
ground transport. Private execution of untouched original instructions matches
5,000 randomized complete direction/blocked outputs; eight numeric references
and branch checks are public. The replay verifies movement, pause, base arrival
near (10.5,10.5), both-bank membership and unchanged resources/allocations.
Team visibility defaults were corrected after independent review.

Validation: 28 native/CLI/graphics CTests, 233 Python regressions and focused
ASan/UBSan steering/transport checks pass. Original instructions/assets remain
private. Capability estimate: 46%, up from 43%, toward a playable single-player
mission. Full dynamics/collision, automatic placement, aircraft, combat/AI, HUD
and mission rules remain. See docs/decomp/native-ground-steering.md.


## 2026-10-14 — original ground velocity dynamics

Continued `e959110` from the latest work branch (main remains behind).
Recovered steering acceleration/drag/external-force gating at 15998..159DC and
FMA velocity integration/horizontal cap at 15C38..15C80/162B0..162FC. Entity +80
is steering acceleration; +84 is horizontal speed limit. Untouched original
instruction comparisons match 10,000 randomized queries / 90,000 float lanes.

Selectable dynamics is wired through simulation/config/replay and viewer N.
The adapter retains live velocity and advances position before computing next
velocity; both-bank migration/accounting remain transactional. Caller collision,
crowds, fixed-altitude policy, immediate orientation and special unit/base state
are not claimed recovered. See docs/decomp/native-ground-dynamics.md.

Validation: 29 native/CLI/graphics CTests, 233 Python regressions, focused
ASan/UBSan checks (host leak inspection unavailable), and actual-level-0
movement/rendering pass. Independent review found no blocking issues.
Estimate: 48% toward a playable single-player mission (previous 46%).
Next: original 7E40 collision/crowd preparation and remaining 158B8 base/height/
special-kind state, then weapons/damage/mission rules. No combat credit.


## 2026-10-14 — original contact response and world containment

Continued `a437a30` / PR #36. Recovered motion-only `7510` contact response
and six-plane `7A50` bounds. Opt-in config/replay contacts and viewer C enable
friendly ground-unit separation and radius-aware containment with dynamics.
Original gather/RNG jitter, terrain/corners, effects, enemy capture and damage
remain excluded; native broad-phase preparation is explicitly distinguished.

Untouched motion-instruction comparisons (effect callback intercepted externally)
match 15,000 randomized queries, 180,000 float lanes and 15,000 return values.
All 30 native/CLI/graphics CTests, 233 Python regressions and focused ASan/UBSan
checks pass; leak inspection is unavailable on this host. Original level-0
movement with two units and 40-model rendering pass. Independent review caught
and resolved coincident index-order signs; both regressions now pass.
See docs/decomp/native-ground-contact.md. Original assets/harnesses stay private.

Estimate: 50% toward a playable single-player mission (previous 48%).
Next original neighbor gathering/jitter and terrain collision, then base/special
state, enemy capture/weapons/damage and mission rules. No combat credit.


## 2026-10-14 — original pair geometry and random contact normals

Continued `9974004` / PR #37. Recovered complete `1F030` random-direction leaf,
`7E40` pair distance/normal/jitter/gap arithmetic and refined mass share. Live
contacts now use the original arithmetic and consume two shuffled-LCG draws for
each eligible native-gather peer, including non-overlaps; failed transactions
preserve RNG, banks and cells. Native gather/scheduling and terrain remain excluded.

Private comparisons match 10,000 random-direction and 15,000 pair queries:
190,000 float lanes and 25,000 next-RNG values, with 1,500 near-coincident cases.
W-zero arithmetic inputs and the exact-zero native guard are explicitly scoped.
Validation: 31 native/CLI/graphics CTests, 233 Python tests (after restoring missing
Capstone), focused ASan/UBSan without host leak inspection, original level-0
movement/rendering, and independent review. No actionable review finding remains.
See docs/decomp/native-ground-pair-preparation.md; original inputs remain private.

Estimate: 52% toward a playable single-player mission (previous 50%).
Next original neighbor gather/scheduling and terrain/corner collision, then
base/special state, hostile capture/weapons/damage and mission rules. No combat credit.


## 2026-10-14 — original terrain walls and corners

Continued `6a2ea7f` / PR #38. Recovered routine-20 local rectangle/four-point
selection and 7970 reference-position point response, composed with 7A50 planes.
Contact mode now reflects normal velocity and permits tangential wall movement.
Old cached cell/XZ reference, obstruction flag and team bits are supplied explicitly;
previous floor0/ceiling2 and conservative route-cell guard remain native policies.

Private original execution matches 10,000 geometry, 10,000 point and 5,000 full
terrain response queries: 380,000 float lanes and 15,000 returns. Public wall/corner,
reference-position and coincident rollback checks pass. All 32 native/CLI/graphics
CTests, 233 Python tests, focused ASan/UBSan without host leak inspection, original
level-0 movement/rendering and independent review pass. Original inputs stay private.
See docs/decomp/native-ground-terrain.md for exact-zero and caller-state boundaries.

Estimate: 55% toward a playable single-player mission (previous 52%).
Next original neighbor gathering/scheduling and height/base/special-kind state,
then hostile capture/weapons/damage, AI and mission rules. No combat credit.


## 2026-10-14 — original height control and gradual orientation

Continued `0cb4995` / PR #39. Added ordinary-ground quaternion direction/bank/
interpolation and height spring kernels; live dynamics now turn gradually.
Contact mode uses original stage/tuning caller gates for floor BE4CCCCC and
ceiling 3F7FFFFF, or alternate world-only 0/2 bounds. Obstruction-controlled
height spring follows terrain correction. Kind 46 retains preview orientation.

Private height comparison: 10,000 queries / 40,000 lanes bit-exact. Orientation:
6,000 queries / 24,000 lanes within 2e-7; 4,216 quaternions bit-exact, maximum
lane error 1.78814e-7. Native zero-cross/zero-direction guards are explicitly
scoped; complete orientation bitwise parity is not claimed. Public live tests
cover gradual turning, envelope gates and upward height steering.
Validation: 34 native/CLI/graphics CTests, 233 Python tests, ASan/UBSan with host
leak inspection disabled, and original level-0 movement/rendering. Independent
review caught and resolved spring-gate and turn-tuning integration errors; no
remaining blocker was found. Original inputs and harnesses stay private.
See docs/decomp/native-ground-height-orientation.md for arithmetic and exclusions.

Estimate: 58% toward a playable single-player mission (previous 55%), a capability
judgment rather than measured source coverage. Next original gather/scheduling,
producer deployment and base/special state; hostile capture, weapons/damage,
mission rules and AI remain major gaps. No combat credit.


## 2026-10-15 — original hostile ownership capture

Continued `166a638` / PR #40. Recovered overlapping stage-5 ground capture by a
hostile ready kind-45 peer: recipient admission precedes source removal, then
owner/link/target bytes change. Live movement commits statistics with banks,
cells and RNG; scene owner display and simulation quota synchronization follow.
Readiness uses an explicit native API/replay control; original automatic weapon
activation, kind-45 victim lifecycle and notification/FX callbacks are excluded.

Private untouched-instruction comparisons match 10,000 cases / 7,680,000 output
bytes and 10,000 capture classifications, including 396 transfers and capacity
rejection. Public tests cover rollback after route/target rejection, malformed
spans, quota conservation, scene ownership, repeated ticks and factory-to-capture
replay. All 35 native/CLI/graphics CTests, 233 Python tests, ASan/UBSan without
host leak inspection and original level-0 capture/rendering pass. Independent
review findings (statistics span overflow and copied-target validation) fixed.
See docs/decomp/native-ground-capture.md; original artifacts stay private.

Estimate: 61% toward a playable single-player mission (previous 58%), a capability
judgment rather than measured source coverage. Next original readiness/weapon
state, producer deployment, gather/scheduling and lifecycle; weapons/damage,
enemy AI and mission win/loss remain major gaps. No damage/mission credit.


## 2026-10-15 — enemy-base impacts and reusable unit capacity

Continued `a95c609` / PR #41. Recovered original 6F40 enemy-base arrival
selection and BF58 resource/base-counter updates. Explicit `base-counter` native
configuration connects movement to base impacts; arriving units unregister,
release quota, leave both cell lists and return their indices FIFO. Scene
projection removal preserves structure links and same-tick index reuse.
Native immediate slot clearing replaces the original asynchronous tombstone
waiting period; notification/FX, corpse behavior and mission outcomes stay out
of scope. Exact-center handling is an explicit native numeric guard.

Private comparison: 10,000 arrival queries (953 hits) match; 10,000 full resource
updates match 1,280,000 output bytes. Public factory-to-impact replay confirms
counter decrement, unit removal, quota release and another production reservation.
Validation: 35 headless / 36 graphics CTests, 233 Python tests, compileall,
repository safety, ASan/UBSan without host leak inspection, and explicitly placed
impact/rendering with original level-0 assets. Independent review found no
blocker; combined capture-to-impact and simulation failure-rollback coverage are
minor follow-ups. Original assets and harnesses stay private.
See docs/decomp/native-ground-base-impact.md for recovered rules and boundaries.

Estimate: 64% toward a playable single-player mission (previous 61%), a
capability judgment rather than source coverage. Next original automatic
placement and weapon/readiness state, health-depleted lifecycle, mission
win/loss and enemy AI. No complete combat or mission-victory credit.


## 2026-10-15 — depleted-health ground lifecycle

Continued `78a8350` / PR #42. Recovered kind-45 victim health depletion on
hostile ready-kind-45 contact; ordinary ground death unregisters once and
becomes kind 62, or requests direct retirement under original flag bit 27.
Recovered original corpse drift/age/velocity/scalar decay and strict expiry.
Owned migration keeps banks/cells consistent; expiry releases the pool index;
scene cleanup and native corpse shrink preview are connected. Original visual
FX, asynchronous kind-63 timing and weapon activation/damage remain excluded.

Private comparisons: 10,000 corpse updates and 10,000 death transitions match
6,400,000 bytes; 10,000 extended capture queries including kind-45 victims match
7,680,000 bytes. Existing base-arrival/resource comparisons remain green. The
public factory-to-contact-to-expiry scenario proves quota release and renewed
production. Original level-0 assets render the temporary corpse (fraction .625)
and verify expiry; 40 models, 11 textures and 10 complete FBOs, three frames.
Independent review caught full-cell corpse migration freezing the entire tick;
a regression now proves age and expiry progress while native drift is blocked.
Final local checks: 36 headless / 37 clean graphics CTests, 233 Python tests,
compileall, repository safety and ASan/UBSan (host leak inspection disabled).
See docs/decomp/native-ground-lifecycle.md for original addresses and boundaries.

Estimate: 66% toward a playable single-player mission (previous 64%), a
capability judgment rather than measured source coverage. Next automatic
producer placement/readiness and original weapon/projectile/damage behavior;
AI, mission win/loss and full original lifecycle scheduling remain incomplete.


## 2026-10-15 — selected base deployment closes the production loop

Continued `d7836fd` / PR #43. Recovered original base-controller release timer,
per-kind/held-input threshold and job-cell center position. Explicit native
`deploy owner kind strength` controls now release reserved ground units without
manual spawn calls, reset timer only on success and project new units after
retired identities are removed. Newborns start moving on the next native tick.
The public two-impact example continuously produces, deploys, moves, hits a base,
releases quota/pool capacity and repeats. Aircraft/full input controller/AI and
original asynchronous scheduling remain excluded.

Original-instruction comparisons: 20,000 timer words/readiness results (8,436
ready), 54 exact-threshold/next-float checks and all 576 four-word cell centers
match. Independent review found no blocker; added multi-owner, full-cell,
same-tick reuse and rejected-clock coverage. Final local validation: 37 headless /
38 graphics CTests, 233 Python tests, compileall, repository safety and ASan/UBSan
(host leak inspection disabled). Original level-0 factory-to-base release and
subsequent movement/rendering pass with 40 models, 11 textures and 10 complete FBOs. See docs/decomp/native-base-deployment.md.

Estimate: 69% toward a playable single-player mission (previous 66%), a
capability judgment rather than measured source coverage. Weapon targeting/
projectiles/damage, player-craft controls, AI, authentic economy/HUD/audio and
mission win/loss remain the main gaps. No full original controller or completed
mission claim.


## 2026-10-15 — interactive selected deployment and status

Continued `ae13373` / PR #44. Native viewer Q/E selects a supported ground kind,
hold D releases its reserved orders through the recovered timer, and Tab keeps
per-player selections. Key-up, player switch, pause, focus loss and viewer exit
cancel held release. Focus regain stays paused. Native on-screen bitmap status
shows funds, active/pending units, selected orders and configured base counters.
Added `interactive-ground-level0.cfg` with explicit synthetic development inputs.
No original input/HUD/combat recovery is claimed. See
`docs/decomp/native-interactive-deployment.md`.

Validation: 38 headless / 39 graphics CTests, 233 Python regressions, compileall,
repository safety and ASan/UBSan (host leak inspection disabled). SDL-injected
controls verify order consumption and cleanup; captured pixels verify status
text. Original level-0 assets render 40 models, 11 textures and 10 complete FBOs;
a private input-driven production/release/movement test passes. Independent
review found no blocker; preserved prior elapsed-time behavior after feedback.

Estimate: 71% toward a playable single-player mission, previously 69%; capability
judgment rather than source coverage. Next: original weapon targeting/damage or
capture activation, then enemy AI and mission outcomes. Original craft controls,
aircraft, economy/HUD/audio/effects and full scheduling also remain incomplete.


## 2026-10-16 — replayable native preview match lifecycle

Continued the latest active PR #45 at `60778fa`, rather than stale main.
The existing outcome display now has an interactive lifecycle: global victory
or draw freezes simulation and cancels all configured deployment strengths;
finished matches reject further gameplay mutations. F2 restores the complete
viewer launch checkpoint paused, including banks/cells, pool, queues/events,
RNG, funds, quotas, counters, timers, motion configuration and scene projection.
Camera and player/type inspection remain available. A three-team elimination
alone does not stop surviving opponents. Restart does not reread assets or maps.

Fixed the existing evaluator silently ignoring missing active base counters,
and made inactive perspectives unconfigured. Its Release test previously lost
all assert checks under NDEBUG; assertions are now enabled and the partial-table
regression was observed failing before the fix.

Validation: all 233 Python tests, 40 headless / 41 SDL/OpenGL CTests, warning-free
Release builds with -Werror, compileall, repository safety and focused ASan/UBSan
(leak detection disabled) pass. Tests drive an actual base impact into a win,
freeze subsequent updates, restore launch state, compare replay entity bytes
including subsequent RNG-dependent spawn, and send real SDL keys to verify
finished-command guards and F2 restoration. Independent review found no blocker.
This session uses synthetic public fixtures; no new original-asset execution,
original-instruction comparison or local Windows execution is claimed.

Estimate: **73% toward a playable single-player mission**, previously 71%, a
capability judgment rather than measured source coverage. This milestone is
native preview lifecycle, not recovery of original campaign win/loss or reset.
Next: original weapons/targeting/projectiles/damage or automatic capture
activation, autonomous enemies, original craft controls and mission progression.
Full-game parity, authentic economy/HUD/audio/effects and aircraft remain open.


## 2026-10-16 — original converter pulse and signed health effects

Continued PR #46 / `06eb1e4`. Recovered kind-45 original phase/readiness pulse
and positive-cooldown state writes; native ground updates now cycle readiness
automatically after motion/contact and before death. No manual readiness seed
is needed for the new capture example. No-target/blocked motion preserves the
full independent pulse clock; review caught and verified that regression.

Recovered signed health accumulation and the original flag-gated, upper-capped
health consumer. Owned per-unit pending effects apply once before the ground
lifecycle, survive rejected/paused updates and restart checkpoints, and cannot
leak through retired index reuse. Added health-effect/replay inspections and a
public automatic capture -> damage/healing -> corpse/expiry scenario.

All 50,000 untouched-original instruction fixtures match: 20,000 converter
pulses, 10,000 cooldown writes, 10,000 full-slot health updates and 10,000
accumulator additions. Full original-level-0 capture/health/death/expiry and
40-model rendering pass (11 textures / ten complete FBOs). Public checks:
233 Python tests, 43 headless / 44 graphics native CTests, compileall, repository
safety, focused ASan/UBSan (host leak detection disabled), independent review.
No proprietary input/disassembly/harness is committed. Exact evidence and
limits: `docs/decomp/native-converter-health.md`.

Estimate: **76% toward a playable single-player mission**, previously 73%; a
capability judgment, not source coverage. Original weapon firing/targeting,
projectile and area-effect producers, enemy decisions, craft controls and
campaign progression remain open. Native queued hits are explicit test inputs,
not an invented automatic weapon. Positive cooldown motion-register damping
remains outside live integration; no supported native producer sets it positive.


## 2026-10-16 — original kind-22 Tesla targeting and beam combat

Continued PR #48 / `2b45108`. An opt-in Tesla source now gathers opposing
ground targets, allocates its original kind-5 beam and applies recovered signed
health effects. Default sustained damage derives from parent +B0=80; kind-46
feedback emits the original extra hit and parent cooldown command. Source and
beam scheduling is deterministic native source-before-beam order, with owned
pool/command/health/RNG state and restart support. The viewer shows a line strip
through the five recovered beam control points.

Private source comparisons match 10,000 full-slot constructors, 10,000 source
gate/state fixtures, 10,000 target scores and 2,208 nonzero cell distances.
Final verification: 51 headless / 52 graphics CTests, 233 Python tests, compileall,
repository safety, nine ASan/UBSan checks and original-asset three-frame smoke
pass. Independent review found no remaining actionable findings. A further
10,512 beam fixtures match full slots, emissions and future RNG draws bitwise.
See `docs/decomp/native-tesla-combat.md` for exact evidence and scope. No private
original bytes, disassembly or harnesses are committed.

Estimate: **82% toward a first playable single-player mission**, previously 79%;
a capability judgment, not measured source coverage. Ground-only Tesla targets,
one beam per source and failed visual-particle allocation are explicit bounds.
Successful original particle effects consume additional shared RNG draws; full
effects/RNG scheduling parity is not claimed. Other weapons/projectiles, enemy
decisions, craft/aircraft and original mission/campaign rules remain open.
