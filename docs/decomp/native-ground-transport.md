# Native ground transport preview

Continues `f5a8538`. Previously deployed ground units remained stationary.
The native world can now advance seven ground kinds (40, 41, 43–47), migrate
records between cells and render their updated positions and headings.
This is an **explicit opt-in transport policy**, not a recovered replacement
for the original movement/steering/collision solver.

## Original evidence and native policy

The exact decrypted NPEB00142 v1.00 ELF was revalidated against SHA-256
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
FE5D8's raw simulation job dispatches the live unit through `18030`: movement
preparation `14060`, update `158B8`, further processing `6F40`, then output DMA.
The original `141A0..141EC` window loads the delta at `26BC0`, position at
context `+480` and velocity at `+4A0`, and executes fused
`position = fma(velocity, delta, position)`. These are job-context offsets,
not newly named fields in the 256-byte entity record.

The new adapter reuses the already recovered target/direction/heading binding
and speed at entity `+84`. It retains a live enemy target instead of randomly
choosing another on every step. A configured target override keeps precedence.
A missing target may be selected through the existing binding helper; only a
successful movement commits its RNG state.

Native policy choices, deliberately kept separate from original evidence:

- Follow the sampled route direction immediately at the configured speed.
- Apply the original FMA arithmetic to x/z; keep altitude fixed.
- Reject blocked route cells, including every crossed intermediate cell and
  both sides of a diagonal corner. This is conservative route obstruction,
  not swept body collision, crowd avoidance or the original corner solver.
- Clamp positions to the configured native extent and refresh cached x/z.
- Preserve the index, kind, statistics, quota, resources and pool.
- Keep original-marker packed records in both banks, preserving the order of
  unrelated records. Update the old/new summary counts on migration.
- A full destination, invalid storage or blocked route leaves the unit unchanged.
- Commit both owned banks/lists synchronously. Original DMA/bank-swap concurrency
  and merge ordering are not reproduced.

Elapsed-time transport runs on a copy of storage and RNG before committing.
Substeps are at most 0.05 seconds and a quarter-cell of travel. Enabled ticks
are limited to ten seconds, and the preview accepts speeds through 64; the
recovered supported units are within that limit. These bounds prevent unbounded
work from malformed preview inputs. The source simulation and transport are
still incomplete at enemy bases; arriving units cannot attack them.

Scene projection now refreshes mobile position/quaternion from the selected
bank after ticks without replacing the structure lookup link in a shared cell.
Native extent is carried from the loaded arena and validated at simulation
entrypoints, including build/event/spawn coordinates.

## Controls and reproducible scenario

Transport defaults to disabled, preserving earlier stationary-spawn scenarios.

| Interface | Action |
| --- | --- |
| Config before `run`: `transport 1` | Enable native transport |
| Replay after `run`: `transport 0` / `transport 1` | Pause/resume transport |
| Replay: `expect-unit-cell index x z` | Assert cached cell and one record in both banks |
| Viewer M | Toggle native transport; window title displays state |
| Viewer T / P | Advance one second / play the simulation |

```sh
build/native/comet_construction_replay < decomp/examples/ground-transport.scenario
```

The synthetic scenario builds a factory, reserves/deploys a unit, advances it
from cell (3,3) to (4,4) and (5,5), disables transport and verifies it remains
there. Balance 340, one active unit and two allocations stay unchanged by
transport. It is not authentic mission balance or automatic producer placement.

## Validation

- Core and owned-scene tests were observed failing before implementation.
- All seven kinds, blocked/no-target/full-cell results, intermediate route
  obstruction, diagonal corners, invalid deltas, rounding restoration, target/RNG
  retention, scene refresh and extent rejection have portable checks.
- A public numeric regression distinguishes fused arithmetic from separate
  multiply/add under round-toward-zero: x `402D0E56`, velocity `BFCCCCCC`,
  delta `3DF9DB22` produce `4020902D`; unfused operations produce `4020902E`.
- A private interpreter runs the untouched original `141A0..141EC` window.
  1,000 finite input fixtures match 4,000 output lanes (seed 141188).
  This checks the position-update window only; it does not establish whole-unit
  physics parity or hardware-perfect SPU floating-point behavior.
- Replay verifies movement, pause and both-bank membership. Offscreen SDL/OpenGL
  checks inject M/T input and verify live movement/projection.

Fresh warnings-as-errors Release passes all 27 native/CLI/graphics CTests and
233 Python regressions. Focused transport/spawn checks pass ASan/UBSan. Original
level 0 unit 10 moved (14.4615,3.40072) -> (15.5929,4.53209) and rendered with
all 40 model slots under offscreen llvmpipe. Windows execution awaits CI.

Proprietary assets, original instructions, disassembly and private oracle stay
outside tracked source.

## Remaining and estimate

The capability estimate is **43% toward a playable single-player mission**,
up from 41%. Two points credit native transport/world migration and its visible
integration. The original full movement solver remains unrecovered: auxiliary
state, acceleration/damping, smoothing, height/special-kind behavior, collision
response, crowds and target transitions. Automatic producer placement, aircraft,
weapons/combat, enemy AI, mission win/loss, HUD/input feedback, income/balance,
audio/save and original shader parity remain outstanding.
