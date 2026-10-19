# Native source-recovery tree

This directory is the new primary implementation track for the Comet Crash PC port.

The goal is **not** to ship the mechanically recompiled PS3 executable behind a large compatibility runtime. The goal is to recover the game's own logic into readable C/C++, assign stable semantics and types, replace PS3-facing subsystems with native PC equivalents, and compile that recovered source as an ordinary Windows program.

The existing `ps3recomp` path is retained as a reversing oracle. It is useful for exact function boundaries, control flow, dynamic traces, asset accesses and behavioural comparison, but it is no longer the target architecture.

## Recovery rules

- Every recovered function keeps its original PPU address in a comment or metadata record until identity is proven.
- Unknown names stay explicit (`sub_XXXXXXXX`, `field_XXXXXXXX`) rather than being guessed into false certainty.
- Raw offsets may be used temporarily when structure layout is proven but field meaning is not. They should disappear as types are reconstructed.
- Recovered source must express game semantics, not emulate PowerPC registers or the PS3 ABI.
- Platform APIs (`cell*`, `sceNp*`, GCM/RSX, SPURS, PS3 filesystem) do not belong in final game-domain source. They are replaced at subsystem boundaries.
- The original static-recomp executable remains useful for differential tests until a recovered subsystem can replace it.

## First recovered code

`recovered/level_map_000D5D5C.cpp` is the first game-specific function translated out of register-level PPU form. It constructs `"%slevel%u.map"`, reads the current level index from the large game state, and delegates to the map loader at `0x000D91B0`.

`docs/decomp/source-anchors.md` records binary evidence that pins several code regions to original source concepts, including the surviving `arenaGraphics.cpp` line markers.

## Ground-unit base impacts

The native loop supports explicitly configured enemy-base counters, original
arrival selection/accounting, unit quota release and FIFO entity reuse. Try
`decomp/examples/ground-base-impact.scenario` with `comet_construction_replay`.
See [recovery evidence and boundaries](../docs/decomp/native-ground-base-impact.md).
Mission counter defaults and victory/defeat rules remain unrecovered.

## Ground-unit health depletion

Recovered kind-45 hostile-contact health depletion now enters the ordinary ground
death/corpse lifecycle, releasing quota once and recycling its slot at expiry.
Try `decomp/examples/ground-lifecycle.scenario`; see
[recovery evidence](../docs/decomp/native-ground-lifecycle.md).
Weapon activation, projectile damage and original tombstone timing remain open.

## Selected base release

`deploy OWNER KIND STRENGTH` configures a held base-release control. Produced
ground units now deploy, move, hit an enemy base and free capacity for further
production without manual spawning. Run `decomp/examples/base-deployment.scenario`;
see [evidence and boundaries](../docs/decomp/native-base-deployment.md).
Original moving-craft/controller input, AI selection and aircraft remain open.

## Interactive ground loop

Launch level 0 with `--arena --construction-config decomp/examples/interactive-ground-level0.cfg`.
P advances production/movement; Q/E selects ground type; hold D releases orders
from the selected player's base; Tab switches player. The native status panel
shows orders, active units and configured base counters. Focus loss/pause/exit
cancel held input. See [controls and limits](../docs/decomp/native-interactive-deployment.md).


## Tesla combat

The interactive ground configuration now enables `tesla 1`: completed kind-22
towers target opposing ground units and create visible kind-5 beam previews.
Run `comet_construction_replay < decomp/examples/tesla-combat.scenario` for
automatic acquisition, damage and death. See
[evidence and limits](../docs/decomp/native-tesla-combat.md).


## Native preview opponent

`opponent OWNER` before `run` enables ordinary factory construction, default
fighter production and automatic release through the existing simulation controls.
Run `comet_construction_replay < decomp/examples/native-opponent.scenario` for
an asset-free encounter ending in base depletion. This is a native preview policy;
original AI recovery remains open. See [behavior and validation](../docs/decomp/native-opponent.md).


## Interactive skirmish launch

Launch with `--assets GAME_DATA --skirmish-config decomp/examples/native-skirmish.cfg --arena`.
This builds a native two-sided battle from configuration and uses original models;
no campaign map extraction is required. The Windows package includes a drag-and-drop
`OPEN_SKIRMISH.cmd` launcher. Player controls skip AI owners and start at the human
base. See [controls, validation and limits](../docs/decomp/native-skirmish.md).

## Gamepad controls

SDL controllers now support the interactive skirmish build/release loop. See
[bindings, recovered stick evidence and limits](../docs/decomp/native-gamepad.md).

F3 enables the [continuous pilot preview](../docs/decomp/native-pilot.md).
