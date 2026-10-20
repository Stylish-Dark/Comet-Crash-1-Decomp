# Original-game recovery status — 2026-10-20

This is a scoped starting inventory, not a complete function audit or a completion
percentage. Prior subjective percentages, including 92%, are withdrawn. Source: the existing recovery documents and PRs #49–58. No new game
behavior was recovered during this correction. Overall effort remaining is unknown.

| Area | Evidence-backed work available | What prevents claiming original-game completion |
| --- | --- | --- |
| Assets and rendering | Original data loaders, model/texture rendering and recovered renderer structures | Complete original scene composition, effects, menus and HUD are not established |
| Construction and ground simulation | Recovered construction, production, movement, contact, capture, base impact and death paths | Full original callers, scheduling, lifecycle and all entity paths remain incomplete |
| Weapons | Selected Tesla, cannon and area-effect paths | Remaining weapon/target families, scheduling and effects remain incomplete; cannon has numeric fallbacks and unproven full-update parity |
| Player craft | Original body slot initialization, selected flight arithmetic, arena containment and ordinary movement-facing | Full constructor registration/player links, controller state/action/carry paths and interaction with the original world remain incomplete |
| Craft numerical fidelity | Scoped instruction comparisons and public fixtures | Input cap uses portable math; exact aligned-Z facing and subnormal/hardware behavior are excluded from matching claims |
| Craft caller policy | Preview moves and renders a ship using recovered leaves | Initial height, mode choice, input lifecycle and control/render ownership contain native policy; deployment remains at the base |
| AI | Working native preview opponent | Original decisions and strategy are not recovered by the preview opponent |
| Missions and campaign | Configured native skirmish, preview outcome/freeze/restart | Original mission rules, campaign progression and complete economy behavior remain unrecovered/incomplete |
| PC controls | Keyboard/controller preview controls and selected recovered input arithmetic | Mouse-capable original gameplay interactions and complete original input/action mapping are not established |
| Audio and persistence | Legacy/runtime evidence and platform scaffolding may be reusable | Complete native original audio triggers/playback, save/load and settings are not established |

## First recovery priority

Close the original player-craft lifecycle and gameplay connection before further
ordinary-flight refinements. Existing evidence links the PPU constructor
CFAB8..CFB88, registration via CA514 and player+220, and SPU type-1 wrapper12B88
through9A70 ->10290 ->6578. The native body initializer does not perform those
registration/player links, and the whole10290 controller is not recovered.

Start with the original registration/player-link and stage/update/commit state
contract so subsequent action/carry branches operate on the actual recovered
entity rather than an independent preview object. Recover the reachable original
logic and preserve ordering/side effects; record unresolved branches explicitly.
Do not fabricate a gameplay meaning for flags just to integrate the preview.

Then recover original action/carry/release paths and their callers; connect them
to the recovered world. Original AI and mission/economy/campaign recovery remain
major subsequent work, not optional polish. Adjust ordering only when actual
source dependencies show a more consequential blocker.

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
