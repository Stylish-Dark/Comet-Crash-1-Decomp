# Original-game recovery status — 2026-10-21

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
| Player craft | Original configured/center spawn, body creation/registration/player links, original9A70 state staging, selected flight arithmetic, arena containment and ordinary movement-facing | Outer bootstrap/configuration selection, full controller state/action/carry and original scheduling remain incomplete; audio requests require an explicit implementation |
| Craft numerical fidelity | Scoped instruction comparisons and public fixtures | Input cap uses portable math; exact aligned-Z facing and subnormal/hardware behavior are excluded from matching claims |
| Craft caller policy | Preview moves and renders a ship using recovered leaves | Initial height, mode choice, input lifecycle and control/render ownership contain native policy; deployment remains at the base |
| AI | Working native preview opponent | Original decisions and strategy are not recovered by the preview opponent |
| Missions and campaign | Configured native skirmish, preview outcome/freeze/restart | Original mission rules, campaign progression and complete economy behavior remain unrecovered/incomplete |
| PC controls | Keyboard/controller preview controls and selected recovered input arithmetic | Mouse-capable original gameplay interactions and complete original input/action mapping are not established |
| Audio and persistence | Legacy/runtime evidence and platform scaffolding may be reusable | Complete native original audio triggers/playback, save/load and settings are not established |

## First recovery priority

Original craft creation,9A70..9EC0 state staging and shared6578 finalization now
have actual storage/player APIs. See original-entity-preparation.md and earlier
creation/finalization notes. Staging matches full context output across10,000
private original-instruction cases; it stops BEFORE1AD8. The complete1AD8 consumer is now recovered separately: pending gate, fresh
reservation, clear/retry/local restore, with another10,000 original-instruction
comparisons. Platform reservation callbacks remain required; automatic record
binding/job ownership is open. Next9EC8..A268 continuation and3510, then full10290
controller/action/carry and source scheduling.
3940/6260 effects/spatial branches, audio and bootstrap remain open; F3 is separate.

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
