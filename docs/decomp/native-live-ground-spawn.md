# Live ground-unit spawning

This milestone builds on `b1c409f`. Reserved orders can now become allocated,
visible ground units in the native simulation. It covers kinds 40, 41 and
43..47, original position jitter, construction/binding, claim/registration and
allocation-failure accounting. Units remain stationary after spawning;
movement, weapons and combat are not implemented.

## Recovered spawn sequence

The new `initialize_arena_ground_unit_spawn` ports `C380..C4DC` and calls the
existing complete ground constructor. Three original RNG draws create x/y/z
jitter. Each integer converts with SPU round-toward-zero, divides by float bits
`4E7FFFFF` using reciprocal `30800000` and the original two FMA refinements.
X/z subtract 0.5, double, and multiply by `3DCCCCCC`; y uses
`fma(ratio, 3C23D710, 3E6B851E)`. The supplied four-vector is then added.
The constructor consumes its existing RNG draw and, when needed, the base
selection draw. Supported inputs preserve exact constructor return codes,
orientation, velocity and unknown reused-slot bytes. Invalid native inputs
preserve RNG and return no template.

`spawn_reserved_arena_ground_unit` composes the recovered operations:

1. Claim the requested pending kind through `BA08`, decrementing pending and
   incrementing active once. No matching order returns `NoOrder` without state
   changes or RNG consumption.
2. Construct the jittered ground unit and register it through `48C8` with
   adding=true, count_active=false. A capacity rejection retains the original
   claim, constructor RNG and statistics partial-update behavior; no allocation
   occurs and the pending order is not restored.
3. Set entity flag hex `+60 | 00800000` and consume one FIFO pool index.
   Write its halfword to hex `+0C`. Unit records use model slot hex `+6F` as
   marker byte 3, rather than the structure insertion marker 40.
4. On allocation pressure, call registration with adding=false and
   count_active=true, matching `C710..C728`. This compensates the active/weighted
   statistics when the capacity gate accepts removal, without restoring the
   order or RNG. It returns `AllocationFailed`.

The implementation computes and validates the constructor on copies before
committing an order. Invalid positions, owners, target configuration and aircraft
return `Invalid` with state preserved. Native numeric exceptions and malformed
storage/pool indices also precede visible writes. This validation order is a
native safety boundary; it does not claim the original validated its caller.

## Native allocation boundary

Original `2058` limits the job's staging record count to 126, consumes an index
through `2AD8`, and packs an eight-byte record with `1F8B0`. `2198` writes the
256-byte entity to the selected output bank. The rest of that SPU job merges
records later. The native single-spawn adapter instead commits synchronously to
both owned banks and both actual-cell lists, then increments that cell's summary
count. It imposes the existing 126-record per-cell safety limit. This is not a
port of SPU batching, DMA concurrency, record migration or pool recycling.

Record arithmetic retains SPU round-toward-zero. The native adapter chooses the
unit's actual clamped cell as the packing origin. Original job cell coordinates
are supplied through `20DC0/20DD0`; the original caller that establishes those
coordinates and the spawn position at `20DF0` remains unrecovered. Center-cell
comparisons below use matching origins. Native jitter across cell boundaries is
covered separately for storage consistency, not claimed as original job-merge
behavior. Construction insertion remains on its existing recovered PPU path.

New owned units start from a zero template. The pure constructor accepts reused
bytes and preserves unknown fields; zeroing new owned slots is native bootstrap
policy. The `B588` notification queue and optional post-spawn metric branch at
`C5E8..C704` remain outside this milestone.

## Simulation, scene and controls

The simulation owns four raw player-statistics blocks initialized from the
configured quotas, with other fields zero. Production reservation updates keep
those blocks' quota fields synchronized; spawning updates the typed quotas from
the registered block. Existing configured active counts are retained, not
inferred from display-only source objects. No spawn funds are charged because
this recovered claim/spawn path does not charge them.

The scene projects position, model and quaternion from the allocated bank.
Mobile records may share a structure cell: they do not replace
`ArenaCell.entity_index`, which remains the structure lookup link. Construction
refresh and elapsed-time ticks skip unit kinds, preserving their bytes and
keeping building-progress overlays off units.

| Operation | Behavior |
| --- | --- |
| `spawn owner kind x z` | Replay: consume a matching order at supplied cell center |
| `expect-live-unit index kind target` | Verify an allocated stage-5 bank slot and its target byte |
| `expect live-units count` | Count allocated unit-kind slots |
| `expect unit-activeN count` | Check player N's active quota |
| U in the native viewer | Spawn the lowest-numbered reserved ground kind for selected owner at the cursor |

These are explicit native placement controls. Factories still reserve orders;
the original automatic producer's timing/selection/placement is not replaced
with a guessed factory-center spawn policy. Existing construction and factory
controls remain available.

```sh
build/native/comet_construction_replay < decomp/examples/live-ground-spawn.scenario
build/native/comet_construction_replay --map /path/to/extracted/data --level 0 < decomp/examples/live-ground-spawn-level0.scenario
```

The synthetic example creates one factory and one unit, then verifies another
order is consumed on pool exhaustion without adding an entity. The level-0
scenario creates four ground units (indices 10..13) from four pending orders.
There are 14 allocated entities, 151 display objects including 137 display-only
source objects, ten ready structures, zero pending units, four active units and
unchanged player-0 balance 280. Positions, tuning, funds and base availability
are explicit native test configuration, not recovered title balance.

## Validation and limits

A private interpreter uses untouched original instructions, ps3recomp commit
`5244db92669b54831c6d3ebb7a96d58c4dbfa6df`, and synchronous DMA:

- 5,000 original `C380` jitter-plus-ground-constructor comparisons match all
  256 bytes, constructor return codes and next RNG outputs (seed 381050).
  Inputs vary reused slots, seven kinds, owners, positions, teams, base
  availability, overrides and cell/pair route bytes.
- Another 5,000 original `BA08` plus full `C380` comparisons match player
  statistics (128 bytes), output-bank slot (256 bytes), packed record (8 bytes),
  return value and next RNG output (seed 3812058). They include successful
  allocation, empty pool, staging count 126 and capacity rejection. Original
  optional post-spawn metrics are disabled; center positions keep original
  packing origin and native actual cell equal.
- Nine committed original numeric reference cases cover all seven ground kinds
  plus pool/staging failure. No original instructions are committed.
- Red/green core, owned-scene, replay and injected viewer-control tests cover
  live projection, quota synchronization, unchanged structure links, later
  ticks preserving units, interior/outer cell crossing, invalid requests and rounding
  restoration. Offscreen OpenGL renders the spawned unit model.

Fresh Release with warnings treated as errors passes all 25 CPU/CLI tests;
all 233 Python regressions and seven affected ASan/UBSan checks pass.
A generated construction-runtime test binary lost execute permission during a
later run; restoring its local permission allowed the complete suite to rerun.
Independent review found no blocking issue; its suggested outer-grid boundary
coverage was added. Offscreen injected build/tick/factory/spawn controls pass.

Private executables, disassembly, interpreter, fixtures and logs stay ignored
under `build`. Comparisons use finite normal floats; denormal/extended-exponent
hardware equivalence, concurrent DMA, omitted notifications/metrics, automatic
producer placement and Windows behavior are not claimed.

The capability estimate is **41% toward a playable single-player mission**,
up from 38%. Three points credit live allocation/failure handling, owned-world
integration and visible deployment. No points are added for movement or combat.
Next: recover the live ground-unit update path (route following, cell migration
and target updates). Original producer placement, aircraft binding, weapons/AI,
mission rules, income/tuning and normal gameplay feedback remain outstanding.
