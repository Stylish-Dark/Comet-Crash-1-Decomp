# Active gameplay continuation

Latest milestone: kind-21 cannon tracking/firing -> kind-3 rounds -> designated
ordinary-ground collision -> deferred kinetic impulse. Scene projection, capacity
retirement and restart checkpoints are integrated. Direct health delta is zero.
See `docs/decomp/native-cannon-combat.md` and the public cannon-combat scenario.
Estimate: **84% toward a first playable single-player mission** (82% prior), a
capability judgment rather than source coverage. Continue from the latest active
PR above the Tesla branch; main remains behind the gameplay stack.

- Next combat recovery: original source terrain/occlusion observer, additional
  damaging projectile families and towers, structure/air targets, effects and
  attribution. Cannon rounds and Tesla beams now operate natively.
- Highest broader playability gap: autonomous enemy decisions using recovered
  construction/production/deployment controls. Moving craft/input and aircraft,
  original mission objectives/progression/rewards and authentic economy remain.
- Finish full stun/cooldown motion, composite weapon rendering, original
  tombstone/output scheduling, HUD and audio.
- Validation: 58 headless / 59 graphics native checks, 233 Python tests, eight
  focused ASan/UBSan checks and original-asset render smoke pass. Full projectile
  instruction comparison remains blocked by an unsupported private interpreter
  opcode; no complete battle/scheduling parity is claimed.

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

## 2026-10-13: native ground transport preview

[x] Opt-in route travel for seven live ground kinds; atomic native cell migration
and both-bank consistency; scene refresh; replay and M/T/P controls.
[x] Original position-update window arithmetic: 1,000 fixtures / 4,000 lanes.
[ ] Recover complete original 14060/13998 steering and 158B8 auxiliary unit update;
native transport does not claim their behavior. Combat/base damage and mission
progression remain the next playability blockers. See native-ground-transport.md.

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
