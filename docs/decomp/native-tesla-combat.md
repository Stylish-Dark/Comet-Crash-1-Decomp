# Original Tesla targeting and beam damage

Continues PR #48 at `2b45108`. The optional native Tesla path lets a completed
kind-22 tower acquire an opposing ground unit, allocate its kind-5 beam and
apply recovered health effects without injected damage. Enable `tesla 1` in a
native construction configuration and enable ground transport. The flag defaults
to false. Construction resources and tuning supplied by a development
configuration remain explicit inputs. The public scenario constructs the tower,
produces an enemy fighter and checks automatic damage/death:

```
comet_construction_replay < decomp/examples/tesla-combat.scenario
```

## Original source and constructor

The canonical ELF SHA-256 remains
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
FE5D8 job SHA-256 is
`9aca1695e6409668d9627ce865ad8008119f1c9e42d81a6106369f5a95e1872f`.
Addresses below are original job addresses. Init `A5F8` selects wrapper `19438`
for kind 22 and wrapper `19EA8` for kind 5. The former calls source `18908` at
`19484`; the latter reaches beam update `19748`.

Source `18908` receives its context in r3. Context `+5D0` points to entity `+80`;
context `+350` is the integer parent-command accumulator. At `18938..1899C`, a
nonzero command with pre-tick entity `+C8 < 0` enters `18E60..18EB4`, which
sets `+C8` to four seconds. The subsequent source gate requires flags bit 24 and
strictly negative `+C8` (`189A0..189C4`). The beam's corresponding parent gate
accepts zero; the source deliberately does not.

| Source field | Recovered behavior |
| --- | --- |
| `+90` halfword | Selected target for beam ordinal zero; `FFFF` means none. Inactive source preserves it. |
| `+B0` float | Beam damage scalar; default kind-22 constructor writes 80. |
| `+B4` float | Adds elapsed while admitted. First acquisition from `FFFF` resets it to zero only above raw `3ECCCCCC` (sound interval). |
| `+B8` halfword | Active beam count; inactive source clears it. Successful allocation increments it. |
| `+BA` halfword | Beam capacity; the supported default is one. |
| `+C4` float | Receives elapsed on an admitted source tick (`189FC`). |
| `+C8` float | Command cooldown, then decremented by elapsed (`18B40..18B84`). |

Job setup `0518..059C` copies root snapshot `+27B0` into global `26BC0`.
The same elapsed scalar therefore drives both the source timer addition and
cooldown subtraction. Source phase/quaternion updates following `18B88`, sound
callbacks and visual effects remain outside the source helper.

Constructor `18F28..190B0` calls original shared initializer `2DF8`. Its inputs
are parent position, owner, index and upgrade byte `+6E`, with beam ordinal zero.
It creates kind 5/model 40/quantity 5/stage 0 at parent position plus raw
`3DCCCCCC` in Y. Initial damage `+3C` is 60, mass scalar `+58` is raw `3C23D70A`,
and health/max health are raw `4E6E6B28` (one billion). Parent scalar `+3C` is
not this constructor's damage input.

Beam `+1E` links the parent; initial target `+1C` is `FFFF`. Five quads at
`+80/+90/+A0/+B0/+C0` start at the elevated source position. Metadata is
`E0=0`, `E4=FFFFFFFF`, `E8=unsigned parent6E`, `EC=F0=F8=0`, and byte `FC=0`.
`E8` is an integer word, not a float conversion. Other untouched bytes are
preserved. Native pool insertion assigns the beam's own index; beams do not
consume ground-unit quotas.

## Target gather and priority

Source `18A64` calls `4428` with stage mask 62, radius from source `+5C`, and
capacity one. Team filtering at `4078..40FC` compares source context `+530`
with the unsigned halfword for candidate owner at snapshot root
`+2720 + 2*owner`; equal teams are rejected. Native configured route teams
supply these inputs.

The current-cell path `4800..48AC` first visits the local 3x3 neighborhood,
Z outermost and X innermost, preserving each packed list's order. Center and
cardinal cells use distance 1; corners use raw `3FB4FDF3`. If it finds any target,
it skips the broader search. Otherwise `4540..4718` scans clipped square bounds
with reach `trunc(radius + raw3EFAE147)`, excludes that 3x3, and admits distance
at most `radius + raw3DCCCCCC`. The general distance is between integer cells,
using `4628..4680` estimate/refinement arithmetic rather than host `sqrt`.

Packed candidate byte 7 contains owner in bits 5..7, stage in bits 2..4 and
category in bits 0..1. After the team/stage gate, score is:

| Candidate | Score |
| --- | --- |
| Marker 36 | `33 + (32 - cell_distance)` |
| Marker 0 | `66 + (32 - cell_distance)` |
| Ordinary stage 5 | `(48 - 3*(route >> 4))*10000 + 110 + category*100 + (32 - cell_distance)` |
| Ordinary stage 4 | `32 - cell_distance` |
| Ordinary stage 1, 2 or 3 | `10` |

The stage mask rejects 0, 6 and 7. Marker overrides precede ordinary-stage
priority. `4184/4188` pass source owner in r5 and candidate owner in r4 to
`1FB20`: it reads the candidate cell's route toward the tower owner, at compact
pair index `3*candidate_owner + source_owner - (source_owner > candidate_owner)`.
This is route priority, not visibility. Replacement is strictly higher score;
exact ties preserve encounter order. Capacity one makes that the highest score
in encounter order; larger-capacity replacement is outside this boundary.

The scorer retains the original admitted stages, but the live adapter gathers
only supported stage-5 ground kinds 40, 41 and 43..47. Structures, bases and
aircraft cannot receive Tesla damage through this native path.

## Beam health effects and native ordering

The beam agent's `19748` recovery validates the linked parent kind 22, ready
flag bit 24 and `parent+C8 <= 0`. A valid existing target is predicted using its
velocity and elapsed. Damage requires predicted distance from the parent
strictly below 2.5. `19CD0` emits signed health delta `-elapsed * beam3C`.
The update then copies parent `+B0` into beam `+3C`: default sustained damage is
therefore **80 per second**, following the constructor's initial value 60.

For kind-46 targets, beam `+F8` adds elapsed during a hit. Strictly above raw
`3E199999` (approximately .15), `19E1C` emits a parent integer command and
`19E4C` emits an additional -1000 health delta. The two health messages retain
their original accumulation order. Parent commands are consumed on the next
native source substep and can start its four-second cooldown.

Native weapon work uses substeps at most .05 seconds. Each substep visits
sources in pool-index order, then beams in pool-index order. Source target
selection and successful beam insertion are visible to that beam pass. A beam
uses its existing target snapshot for its current hit, then copies the parent's
selected target for its next update. Parent counts are synchronized from live
beam links; failed pool/cell insertion preserves the original unassigned target
and sound-timer behavior. Retirement returns the native pool slot synchronously.

This ordering is deterministic and replayable, but does not reproduce original
SPU DMA snapshots, output queues or concurrent job scheduling. Ground movement
runs before the weapon substeps; those weapon substeps do not re-run movement.
Health deltas flow into the existing signed-health and death/corpse consumer.
The simulation transaction owns banks/cells, pool, pending health, parent
commands and RNG together, including pause/rejection/restart state.

## Numeric, effect and rendering limits

The helpers retain round-toward-zero arithmetic and restore the caller's
rounding mode. Native source admission is kind 22/stage 4/capacity one, with
finite scalar inputs and elapsed in `[0,3]`. The beam helper admits elapsed in
`(0,.25]`, finite normal-or-zero scalar operands, bounded positions/velocities,
and bounded timers/damage. The adapter uses the tighter .05 substeps. General
cell distance rejects zero and offsets outside `[-23,23]`; the original broad
scan excludes zero. These are native validation boundaries, not claimed
original input guards.

Original hit-effect allocation is an explicit beam-helper input. With no
particle backend, the live adapter models **allocation failure**. Successful
original hit particles consume five extra shared RNG draws before beam jitter;
that branch can be tested, but the live path does not reproduce successful
particle allocation, its visual result or the resulting whole-game RNG sequence.
Other omitted source callbacks also prevent a complete original-effects claim.

The native viewer draws a line strip through the five recovered control points
at beam `+80..+C0`. It is a trajectory preview, not recovered original lightning
shaders, ribbon geometry, particles, sound or composite weapon rendering.

## Validation

Private untouched-instruction comparisons completed for the source boundary:
10,000 full-256-byte constructor fixtures, 10,000 source gate/state fixtures,
10,000 candidate-score fixtures and all 2,208 nonzero grid offsets within
`[-23,23]`. Source comparisons supply target-gather results and successful
allocation at their external boundaries and suppress callbacks; the arithmetic,
gates and owned state writes execute unchanged. Constructor comparisons include
the original shared initializer. Seed, harnesses and original bytes remain
private and are not required by the public build.

Portable source tests cover strict-zero/crossing cooldown, acquisition sound
threshold, inactive target preservation, invalid capacity, constructor metadata
and preserved bytes, and rounding restoration. Public integration tests exercise
constructed tower -> automatic beam -> health -> death, pending-command cleanup,
allocation failure and restart replay without queued health injection. Final verification passes **51 headless / 52 graphics CTests**, 233 Python tests,
compileall, repository safety, and nine focused ASan/UBSan tests (host leak
inspection disabled). Graphics checks include visible beam pixels and original
level-0 assets: 40 models, 28,890 vertices, 10,202 triangles, 11 textures, ten
complete framebuffers and three rendered frames. Independent review found no
remaining actionable findings. An asymmetric-route test checks candidate-to-tower
priority; local-first gathering, allied filtering, full-pool failure, strict hit
range, medic shutdown/restart and paused state are exercised.

A further **10,512 beam fixtures** (10,000 randomized and 512 boundaries) match
all 256 output bytes, ordered health emissions, parent commands, retirement and
41 future RNG draws bitwise. Both successful and failed hit-particle allocation
branches execute their original effect instructions; only the live native adapter
chooses allocation failure.

Missile/bullet firing, the kind-2 projectile path, other towers, multi-beam
upgrades, hostile structure damage, enemy decisions, craft/aircraft controls and
original mission/campaign rules remain open.
