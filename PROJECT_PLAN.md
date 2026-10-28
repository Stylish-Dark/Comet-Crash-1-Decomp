# Comet Crash completion plan

Updated 2026-10-22 (Australia/Sydney). Planning baseline: PR #63,
`43075bcb22b4e4be2323f82f8344dfa160810f2d`. This is the authoritative execution
map. STATE.md records the latest checkpoint; WORK_QUEUE.md identifies the next
work item. Historical session entries and preview percentages cannot override it.

## Deliverable and current position

Deliver the original Comet Crash NPEB00142 v1.00 as readable recovered C/C++,
built into a native Windows game with mouse/keyboard and controller support.
Preserve original gameplay, calculations, decisions, update order and content.
Recover original behavior first; adapt platform interfaces explicitly. A similar
skirmish, original models on screen or a collection of matching functions is
not the finished game.

A native preview and substantial recovered code exist. The original player loop
is incomplete; no complete faithful original mission has been accepted. Most
original AI/campaign and full presentation/audio/persistence work remains.
The immediate bottleneck is connecting the original player controller and its
state/request/update dependencies into the running game.

Overall planning estimate remains **about25%, plausible15–35%** for the complete
faithful PC game. This is a coarse engineering estimate, not an hourly meter.
This plan adds no recovered game code and earns no increase. Do not manufacture
an effort percentage from task counts, test counts or original instruction bytes.

## How to use this map

Work items have stable IDs. **Done** means only the stated scoped deliverable is
supported by the linked evidence. **Partial** means useful source exists but the
row's completion criterion is unmet. **Open** means no accepted implementation of
that original capability is established. An acceptance gate stays unchecked until
its runnable demonstration exists; completing helpers alone does not pass it.

Dependencies establish an initial order, not a license to spend sessions mapping
unrelated systems. Trace missing calls while implementing the selected item. Add
newly discovered work under its existing parent, record the source/dependency and
its effect on the forecast, and preserve IDs/history. Never hide scope growth by
renumbering the remaining work. A necessary prerequisite may run first; connect it
back to its parent immediately after it is checked. Reuse passing work.

## Playable checkpoints

| Gate | Current status | What must be demonstrable |
| --- | --- | --- |
| G0 — Native preview foundation | Available, scoped below | Windows native build/package, original asset/model loading, rendered preview and existing recovery tests. This establishes tooling and a preview, not faithful gameplay. |
| G1 — Original player loop | Not accepted | On a reference arena, use the actual registered craft to move, collide, take damage, pick up/carry/release and perform its original actions. Render committed authoritative state with recovered update order. Mouse commands feed the same original action semantics. |
| G2 — First original mission | Not accepted; immediate playable-game target | Load a named original campaign mission from original data, play it from start to original success/failure using original player actions, economy, opponents and mission rules; restart and finish again. Required unit/weapon paths and minimal usable feedback must work. |
| G3 — Complete original content and modes | Not accepted | All reference missions and supported modes run with their required entity/weapon/AI/objective families and original progression. Reference content/mode inventory must be explicit. |
| G4 — Complete game experience | Not accepted | Original menus/HUD/effects/audio, save/load/settings, faithful PC input and all reference multiplayer/mode behavior are integrated. Final numeric/platform differences are resolved or identified as release blockers. |
| G5 — Faithful Windows release | Not accepted | Package installs/starts with user-supplied original data, complete playthrough/mode acceptance succeeds, controls and persistence work, and no known replacement gameplay or unresolved fidelity blocker remains. |

G1 -> G2 -> G3 -> G4 -> G5 is the acceptance order. Audio, presentation and
persistence may be developed earlier when a dependency requires them; final
acceptance must not postpone essential original behavior under the label polish.

## Existing foundation: reuse it

| ID | Status | Scoped result and evidence |
| --- | --- | --- |
| F1 | Done | Exact reference pinning and original PPU/SPU analysis/comparison tools. [SPU inventory](docs/decomp/arena-spu-jobs.md), source metadata under docs/decomp. This is not an inventory of every recovered function. |
| F2 | Done | Original level-map and core OBJ/MTL/DDS loading. [Map format](docs/decomp/level-map-format.md), [asset/viewer scope](docs/decomp/native-viewer.md), level_map/model_geometry/dds_image modules. Remaining formats/presentation belong to V1/V2. |
| F3 | Done | Native C++ executable, SDL/OpenGL preview, Windows build/package and keyboard/controller preview input. [Viewer](docs/decomp/native-viewer.md), [gamepad](docs/decomp/native-gamepad.md), native-source-port workflow. Full original input is P7/P8. |
| F4 | Done within documented boundaries | Original craft body creation/registration,9A70 preparation/accumulator consumption/completion and6578 finalization APIs. [Creation](docs/decomp/original-craft-registration.md), [preparation](docs/decomp/original-preparation-effects.md), [finalization](docs/decomp/original-entity-finalization.md). Full controller/death/scheduling remain P1–P6. |
| F5 | Done for serial publication | Original1E090/2AD8 counted-request publication with FIFO order, batch32/reserve128, partial exhaustion and metadata. [Publication](docs/decomp/original-request-publication.md). Original bootstrap and downstream consumers remain P5. |

F4/F5's numeric and dependency exclusions stay explicit. Their status does not
accept G1, claim compiler matching, or grant whole-system parity.

## G1 work: finish and connect the original player

| ID | Status at baseline | Implementation and acceptance criterion | Dependencies / existing evidence |
| --- | --- | --- | --- |
| P1 | Partial — ordinary/banking and grid approach connected | Recover full10290 controller dispatch, original snapshot/control reads and state branches. Account for all reachable branches/callees; unsupported branches remain named blockers. Connect already recovered flight/facing instead of replacing controller decisions with preview flags. | F4/F5; arena_craft_flight/facing; [flight](docs/decomp/native-craft-flight.md), [facing](docs/decomp/native-craft-facing.md). Source wrapper12B88 invokes9A70 ->10290 ->6578. [Connected ordinary controller](docs/decomp/original-player-control.md) now executes this path into real banks/scene; ordinary actions/target-facing and mode0 attached positioning are connected; [banking/proxy lifecycle, automatic grid approach and positive attachment countdown](docs/decomp/original-player-banking.md) now execute; [full-action particles and hard craft containment](docs/decomp/original-player-action-effects.md) now execute; [expired attachment growth/repair and completion](docs/decomp/original-attachment-states.md) now execute; [real-peer contact and first health displays](docs/decomp/original-player-health-contact.md) now connect; capture/numeric and death branches remain. |
| P2 | Partial — controller decisions/publication recovered | Recover pickup, carrying, attachment, release and other controller action branches. Requests must change the correct original world/player state, including rejection and interruption. Demonstrate each action in a connected scenario. | P1, P5; [ordinary action and target-facing recovery](docs/decomp/original-player-actions.md). Kind18 is a visual consumer, not the carried-object mutation path. Interaction-arm proxy construction/lifecycle and positive attachment countdown are recovered. Expired attachment growth/repair and completion requests are connected for supplied source states. Actual pickup/link and drop/unlink transitions and carried-object updates remain missing. |
| P3 | Partial | Complete controller-gated world/peer contacts, facing/banking and attachment response; exercise collisions in the actual player context. Preserve original flag gates. | P1; existing containment, ground-contact and aim helpers; original target-facing and nonzero banking now connected; full-expansion11A80 particles and hard7DD0 craft containment now connect through bank commits; single-peer player contacts now connect through7E40/7510, impulse publication/consumption and bank/scene commits; capture/numeric boundaries remain.3238 is a flag8000 request path, not a terrain-query substitute. |
| P4 | Partial | Complete player damage/death/removal/reset lifecycle and its action/carry cleanup. Demonstrate lethal and nonlethal paths; preparation health changes alone do not close this item. | F4, P1/P2/P5; existing accumulator, preparation and finalization code;3940 first-display allocation/publication and existing-display return are recovered; downstream display consumer, full death/reset and6260 remain. |
| P5 | Partial | Recover request queue initialization, downstream consumption/dispatch, allocation and resulting state commits needed by player actions. Preserve source ordering, reserve/failure and ownership. Publish -> consume -> observable world change must run. | F5 supplies publication only; trace original consumers and bootstrap. |
| P6 | Partial | Recover snapshot ownership, original simulation scheduling/order and bank commit around the player loop. Bind9A70 ->10290 ->6578 to registered storage; feed renderer from committed body rather than independent NativePilot state. | P1–P5; Connected ordinary10290 now joins stage/complete/finalize APIs and renderer-facing scene. Original B500 reference fetch now reads real bank/accumulator records. Original scheduling/input remain missing; native preview scheduling is not acceptance. |
| P7 | Partial | Recover complete original command/button lifecycle and controller abstraction. Verify press/hold/release/cancel through actual player actions. | P1/P2/P6; existing snapshot command evidence and native gamepad adapter are foundations. |
| P8 | Open | Map mouse/keyboard to recovered commands/actions, including picking/position conversion where required. Demonstrate G1 actions with mouse and keyboard; retain controller support. | P7; platform adaptation must preserve original gameplay decisions. |

Execution order: P1 first; recover P2/P3/P4 branches with the P5 dependencies they
actually reach; P6 connects the resulting complete update; P7/P8 complete input.
Do not delay integration until every unrelated entity family is recovered.

The first P1 checkpoint must implement a connected controller branch from entry
through its real state writes, with an explicit list of remaining controller
branches/callees. It is an intermediate source checkpoint, not G1 acceptance.
Following checkpoints must remove that list's blockers and then execute the full
player loop. Publishing another standalone numeric helper is insufficient unless
a concrete failing original path requires it.

Latest P1/P6 checkpoint: [original reference fetch and attached positioning](docs/decomp/original-player-references.md). P2 remains Partial; the prior kind18 release-consumer interpretation is corrected. G1 remains unaccepted.

Previous P1 checkpoint: [ordinary controller and remaining branches](docs/decomp/original-player-control.md). Provisional G1 remaining effort16/32/64 focused hours (low/likely/high), low confidence and uncalibrated; assumptions/breakdown are in that checkpoint. G1 remains unaccepted.

## G2 work: make one original mission playable

| ID | Status at baseline | Implementation and acceptance criterion | Dependencies / existing evidence |
| --- | --- | --- | --- |
| M1 | Partial | Select and record the first original campaign mission using the reference's own selection/order; recover its bootstrap, player/enemy setup, initial resources, data and script bindings. Load/restart it without synthetic scenario policy. | F2, P6; native_session and [bootstrap](docs/decomp/native-bootstrap.md) exist but do not establish authentic mission initialization. |
| M2 | Partial | Finish original economy/building/production/deployment paths needed by that mission, invoked by the actual player and enemy state. Verify costs, rejection, completion, quotas and resource transitions. | P2/P5/P6, M1; arena_construction*, arena_factory_*, arena_routes, arena_unit_spawn and related recovery docs. |
| M3 | Partial | Complete that mission's required ground/air/entity movement, targeting, contacts, capture, damage and lifecycle. List required types from original mission data and close each required original update path. | M1/M2; existing arena_ground_* and entity/unit source. Unused families continue under C1. |
| M4 | Partial | Complete every weapon/target/effect family used by the selected mission, including source update order and failure/cleanup. | M1/M3; existing Tesla/cannon/laser/projectile/area-effect code has stated limits. |
| M5 | Open for original AI | Recover the original enemy decisions, timers and action scheduling required by the mission. Replace opt-in native_opponent policy in the faithful path. | M1–M4; [preview opponent](docs/decomp/native-opponent.md) is evidence of a replacement, not original AI completion. |
| M6 | Open for original objectives | Recover mission objectives, success/failure, transitions, rewards and restart. The original mission must finish from actual gameplay; native base-counter outcome rules do not qualify. | M1–M5; [preview outcome](docs/decomp/native-mission-outcome.md) documents the current substitute. |
| M7 | Partial | Supply the mission's usable camera, object state, selection/action feedback and essential HUD. Render original data and authoritative simulation; no invisible required interaction. | P8, M1–M6; native_construction_scene/viewer and recovered renderer structures. Full presentation continues under V1. |
| M8 | Open | Package and demonstrate the named original mission: start -> player actions/build/economy -> enemy interaction -> original success/failure -> restart. Record exact launch command, executable artifact, original-data manifest and remaining exclusions. | G1 and M1–M7; this is G2 acceptance, not another synthetic replay. |

Prioritize the smallest real original mission dependency set. M3/M4/M5 do not
require every campaign family before G2, but every family used by that mission
must be original. The selected mission and its required types must be recorded
before claiming a bounded mission forecast.

## G3–G5 work: complete the original game and release

| ID | Status at baseline | Implementation and acceptance criterion | Dependencies |
| --- | --- | --- | --- |
| C1 | Partial | Inventory all original entity/weapon families and finish their complete callers/lifecycle, including aircraft and paths absent from G2. Link each type to source, implementation and acceptance evidence. | M3/M4, P6. |
| C2 | Open | Recover remaining original enemy behavior, mission-specific rules/scripts, campaign unlocks/rewards and complete progression. Inventory all reference missions and prove each completable. | M5/M6, C1. |
| C3 | Open | Inventory all reference game modes/player configurations; implement original local/multiplayer behavior and mode-specific rules. Record required PC substitutions for platform services explicitly. | P7/P8, C1/C2; reference determines content, not guesses. |
| V1 | Partial | Complete original scene composition, camera, menus, HUD, fonts, effects/materials/postprocessing and feedback across content/modes. Existing models/targets do not establish complete presentation. | F2/F3, M7, C1–C3; renderer/asset evidence. |
| V2 | Open for complete native path | Recover all original audio events, voice/music selection, sequencing and native playback. Preserve event semantics and timing; silent callbacks are not acceptance. | P4/P5, M1–M6, C1–C3. |
| S1 | Open | Recover save/load/settings/progression representation and native filesystem integration. Restart executable and restore the correct state; exercise failure/recovery paths. | C2/C3, P7/P8. |
| Q1 | Partial | Resolve known fidelity exclusions: portable input cap, aligned-axis/numeric limits, subnormals, remaining unsupported branches and native scheduling/policies. Maintain discrepancy list with evidence; close material differences before final acceptance. | Relevant recovered systems; see [fidelity status](docs/decomp/fidelity-status.md). |
| Q2 | Partial foundation | Finish Windows packaging/data import, supported controls/display/timing, configuration and shutdown. Run complete content/mode acceptance and fresh-start/save-resume checks with a pinned build. Deliver executable artifact and exact instructions. | G3, V1/V2/S1/Q1. Existing CI packaging is reusable foundation. |

## Verification and progress reporting

A source item needs its original entry/caller, decisions, state writes and return
or commit path, plus proportional source/output evidence. An integrated item
also needs a connected runtime demonstration. Comparison cases establish their
stated domain; they do not establish whole-game or matching-compilation parity.
Use one appropriate build/test pass after relevant changes. Broaden only for
changed dependencies, failures or unresolved concerns. Do not rerun passing
suites, refine accepted math or expand documentation to manufacture activity.
Documentation-only changes require link/status consistency checks, not game builds.

Every implementation checkpoint must report:

- Parent work-item ID and playable gate it advances.
- What changed in original behavior, and what can now run that could not before.
- Before/after item status, remaining branches/dependencies and next action.
- Verification result and executable/demo link when a runnable gate changes.
- Implementation versus restoration/build/verification time when observed; do not
  reconstruct invented timing after the fact. Record blockers that consumed work.
- Rounded full-deliverable estimate with its basis/range, separately from task
  status. A flat rounded estimate must accompany concrete ledger movement, not
  merely a larger test count. A completed helper does not imply1% of the port.

Estimate remaining effort per bounded item only after inspecting its actual
source and integration dependencies. Record low/likely/high estimates, assumptions
and actual elapsed work; revise when new source changes scope. Use completed,
comparable items to calibrate forecasts. Currently no calibrated completion date
exists; do not invent one. The next P1 source pass must record its reachable
branch/dependency breakdown and a provisional G1 forecast alongside code, rather
than spend another session on a project-wide estimating exercise.

The owner has authorized continuation. Execute the next task without asking for
another approval/continue, unless a real blocker or user decision prevents it.
Checkpoint when execution capacity ends. Do not claim the game is nearly done
until the acceptance table shows complete original gameplay/content running.

## Architecture and implementation constraints

Recover semantic readable C/C++ with source addresses retained as provenance.
Keep platform graphics/audio/input/filesystem/threading behind native interfaces.
The private PS3 executable and ps3recomp are analysis/comparison tools, not the
shipping game architecture. Use original user-supplied assets; commit no original
binary/disassembly, Sony SDK material or proprietary game data. Preserve useful
existing recovery and preview tooling. Do not auto-merge the existing PR stack.
