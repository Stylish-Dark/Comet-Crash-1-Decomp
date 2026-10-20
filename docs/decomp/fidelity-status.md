# Original-game recovery status — 2026-10-20

This is a scoped starting inventory, not a complete function audit or a completion
percentage. Prior subjective percentages, including 92%, are withdrawn. Source: the existing recovery documents and PRs #49–58. The initial correction recovered no game code; the later creation milestone below does. Overall effort remaining is unknown.

| Area | Evidence-backed work available | What prevents claiming original-game completion |
| --- | --- | --- |
| Assets and rendering | Original data loaders, model/texture rendering and recovered renderer structures | Complete original scene composition, effects, menus and HUD are not established |
| Construction and ground simulation | Recovered construction, production, movement, contact, capture, base impact and death paths | Full original callers, scheduling, lifecycle and all entity paths remain incomplete |
| Weapons | Selected Tesla, cannon and area-effect paths | Remaining weapon/target families, scheduling and effects remain incomplete; cannon has numeric fallbacks and unproven full-update parity |
| Player craft | Original configured/center spawn, body creation/registration/player links, selected flight arithmetic, arena containment and ordinary movement-facing | Outer bootstrap/configuration selection, full controller state/action/carry/commit and original scheduling remain incomplete; audio requests require an explicit implementation |
| Craft numerical fidelity | Scoped instruction comparisons and public fixtures | Input cap uses portable math; exact aligned-Z facing and subnormal/hardware behavior are excluded from matching claims |
| Craft caller policy | Preview moves and renders a ship using recovered leaves | Initial height, mode choice, input lifecycle and control/render ownership contain native policy; deployment remains at the base |
| AI | Working native preview opponent | Original decisions and strategy are not recovered by the preview opponent |
| Missions and campaign | Configured native skirmish, preview outcome/freeze/restart | Original mission rules, campaign progression and complete economy behavior remain unrecovered/incomplete |
| PC controls | Keyboard/controller preview controls and selected recovered input arithmetic | Mouse-capable original gameplay interactions and complete original input/action mapping are not established |
| Audio and persistence | Legacy/runtime evidence and platform scaffolding may be reusable | Complete native original audio triggers/playback, save/load and settings are not established |

## First recovery priority

The original craft creation path is now recovered and exposed through the actual
simulation storage/player registry; see original-craft-registration.md. Next close
the original9A70 staging, full10290 state/action/carry controller and6578 commit
with source caller/scheduling semantics. Creation alone does not connect the F3
preview or complete craft gameplay. Original audio and outer bootstrap gates
remain unresolved; do not silently substitute them.

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
