# Original-game recovery status — 2026-10-21

Latest player checkpoint: [reference fetch and mode0 attached positioning](original-player-references.md)
connects original B500 bank/accumulator reads and10648 position selection through
real scene commits. Attachment links are explicit test fixtures; original pickup,
drop, banking, lifecycle and scheduling are still incomplete. Kind18 is a visual
consumer, correcting the earlier release-world-effect interpretation. G1/G2 are
unaccepted.

This is a scoped inventory, not a complete function audit or measured coverage.
Prior subjective percentages, including92%, remain withdrawn. The new overall
planning estimate is about 25% (plausible 15–35%); see completion-assessment.md for
its basis and limits. Source: recovery documents, PRs #49–59 and the shared
entity-finalization/preparation milestones.

| Area | Evidence-backed work available | What prevents claiming original-game completion |
| --- | --- | --- |
| Assets and rendering | Original data loaders, model/texture rendering and recovered renderer structures | Complete original scene composition, effects, menus and HUD are not established |
| Construction and ground simulation | Recovered construction, production, movement, contact, capture, base impact and death paths | Full original callers, scheduling, lifecycle and all entity paths remain incomplete |
| Weapons | Selected Tesla, cannon and area-effect paths | Remaining weapon/target families, scheduling and effects remain incomplete; cannon has numeric fallbacks and unproven full-update parity |
| Player craft | Original configured/center spawn, body creation/registration/player links, original9A70 preparation, counted requests and original1E090/2AD8 publication with explicit native ownership/transport boundaries, selected flight arithmetic, arena containment and ordinary movement-facing | Outer bootstrap/configuration selection, full controller state/action/carry and original scheduling remain incomplete; audio requests require an explicit implementation |
| Craft numerical fidelity | Scoped instruction comparisons and public fixtures | Input cap uses portable math; exact aligned-Z facing and subnormal/hardware behavior are excluded from matching claims |
| Craft caller policy | Preview moves and renders a ship using recovered leaves | Initial height, mode choice, input lifecycle and control/render ownership contain native policy; deployment remains at the base |
| AI | Working native preview opponent | Original decisions and strategy are not recovered by the preview opponent |
| Missions and campaign | Configured native skirmish, preview outcome/freeze/restart | Original mission rules, campaign progression and complete economy behavior remain unrecovered/incomplete |
| PC controls | Keyboard/controller preview controls and selected recovered input arithmetic | Mouse-capable original gameplay interactions and complete original input/action mapping are not established |
| Audio and persistence | Legacy/runtime evidence and platform scaffolding may be reusable | Complete native original audio triggers/playback, save/load and settings are not established |

## First recovery priority

Original craft creation,9A70 staging/consumption/completion and shared6578
finalization have actual storage/player APIs. See original-preparation-effects.md
and earlier preparation/creation notes.10,000 full9A70 type1 cases match context
and counted requests at the explicit1E090 publication boundary. Reservation/retry
behavior is separately covered in PR #61. Full10290 controller/actions/carry,
original request consumption, queue bootstrap, snapshot/job ownership and
scheduling remain next. Recovered1E090/2AD8 now supplies request publication.
Numeric exclusions,3940/6260, audio/bootstrap and full death lifecycle remain open.
F3 remains separate.3510 constructs counted requests; it is not a complete death
handler. Output comparisons do not establish matching compilation/hardware parity.

Then complete original action/carry/release integration and the remaining original
AI, mission/economy/campaign paths. These are essential recovery, not polish.

## Evidence boundaries that must survive

- 3238 is evidenced as a flag8000-gated kind7 request/queue path, not a proven
  general terrain query. Peer contact1162C ->7E40 is gated by entity+51mask1C.
- The flight oracle checked selected integration arithmetic, not the full10290
  controller or its complete post-step position/height behavior.
- Matching outputs for selected cases does not prove original hardware parity,
  full-controller parity, complete scheduling or whole-game fidelity.
- Native opponent, skirmish rules, synthetic funds and preview lifecycle policies
  are useful test scaffolding and remain replacement behavior.

See native-player-craft-initialization.md, native-craft-flight.md,
native-craft-facing.md, native-opponent.md, native-skirmish.md and
native-cannon-combat.md in this directory for evidence and specific exclusions.

Request publication is recovered through1E090/2AD8 with serial native FIFO/pool
ownership; see [scope and evidence](original-request-publication.md). It leaves
queue bootstrap, original downstream consumption and scheduling open.

2026-10-23: [ordinary action decisions and target-facing](original-player-actions.md)
now connect into registered craft commits and1E090 request publication. Kind18
consumption, carried-object world changes and complete player-loop acceptance
remain missing. See the current completion map; do not treat old branch lists as
evidence that the new action decisions are still unimplemented.
