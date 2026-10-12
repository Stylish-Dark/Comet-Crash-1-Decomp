# Original entity registration and player statistics

This milestone builds on ground constructor commit `126a645`. It ports the
original FE5D8 routines `BB30..BF54` and `48C8..4B64` to native C++20. These
are the statistics operations between production claims and live allocation.
They do not allocate, move, render or fight a live unit.

## Statistics delta

`apply_arena_player_statistics_delta` accepts a 128-byte big-endian player
statistics block and typed deltas. Unknown bytes are preserved. Offsets below
are decimal; offset names remain where a game-domain meaning is unverified.

| Field | Update |
| --- | --- |
| Signed halfword 90 | Active count plus signed active delta, if capacity accepts |
| Float 64 | Ground weight plus delta, if capacity accepts |
| Float 68 | Air weight plus delta, if capacity accepts |
| Halfword 86 | Count delta, wrapping at 16 bits |
| Bytes 92, 93 | Byte deltas, wrapping at 8 bits |
| Unsigned halfword 122 | Add unsigned auxiliary byte and cap at 250 |
| Floats 0, 4, …, 60 | Add sixteen corresponding scalar deltas |
| Float 116 | Add scalar delta and clamp to [2, 16] |

Capacity accepts when `signed(active_90) + unsigned(pending_94) + signed(delta)`
is at most the **signed 32-bit** capacity at 124. Equality succeeds; negative
capacity words retain their signed meaning. A false result skips only active
count and weighted writes. All other updates still happen, including timer
clamping with a zero timer delta. Capacity rejection is not a transaction abort.

Halfword/byte wrap is intentional. The native port owns its block directly;
original atomic DMA read/commit/retry scheduling is outside this boundary.

## Entity registration

`register_arena_entity_statistics` reads the entity's packed stage at hex `+33`.
Adding uses +1; removing uses -1. Stage 5 changes ground weight, stage 6 changes
air weight, and stages 3/4 change count 86. Weight is the entity float at hex
`+50` multiplied by that sign. Active delta is included only for stages 5/6
when `count_active` is true. Auxiliary input is unsigned even on removal.

Stages 0/1/2/7, entity type 1, and entities with hex `+60 & 00080000` return
`UnsupportedEntity` without touching statistics. Supported registrations return
`Registered` or `CapacityRejected`, preserving the latter's partial-update rule.
The registration routine consumes no RNG.

The original spawn helper `C380` calls registration with adding=true and
count_active=false: `BA08` has already incremented active count. Its allocator
failure branch at `C710` calls removal with count_active=true. The committed
integration test connects the existing claim helper to both registrations and
checks the count, weights and pending-kind state. It demonstrates the accounting
sequence, not execution of the full spawn helper or producer failure path.
The pending order is not restored by these operations.

`read_arena_unit_production_quota` and `store_arena_unit_production_quota` bridge
the existing typed quota to the raw block at 90, 94, 96..112 and 124. Storing
preserves unrelated fields; the codecs retain all raw capacity bits and count
wrap. The factory timer's existing supported quota-domain guards still apply.

## Numeric boundary and validation

Arithmetic uses host round-toward-zero with contraction disabled. The caller's
rounding mode is restored on success and exceptions. The native policy flushes
subnormal inputs/results to positive zero, consistently with the existing SPU
recovery leaves. Nonfinite operands and results outside the supported normal
IEEE float range throw `invalid_argument` before changing the block. Unlike
capacity rejection, this native unsupported-numeric boundary is atomic. Fields
skipped by the capacity gate are not read for their arithmetic.

A private ps3recomp interpreter at commit
`5244db92669b54831c6d3ebb7a96d58c4dbfa6df` executes the untouched original
routines with synchronous DMA. All 10,000 complete 128-byte results and return
codes match native execution (seed 481130; 5,000 delta cases and 5,000 entity
cases). Inputs vary reused bytes, signed/unsigned counts, negative/equal/positive
capacities, weights, scalar deltas, stages, types, excluded flags, add/remove,
count-active, auxiliary bytes and all four player offsets.

Those comparisons cover normal floats and signed zeros. The interpreter's
`fa`/`fm` helpers use host arithmetic and do not reproduce SPU denormal flushing;
an initial denormal comparison exposed that limitation. Denormal handling is
therefore tested as native policy separately, not claimed as an original-runtime
comparison. NaN, infinity, extended exponents and concurrent DMA retries are
outside the oracle claim. Original bytes, disassembly and private harnesses
remain ignored under `build/recovery`; fourteen committed reference cases hold
only resulting numeric state and synthetic input data.

Regression tests also cover complete-block preservation, capacity rejection's
side effects, both counter widths, all stages, blocked entities, quota round
trips, claim/register/removal composition, numeric rejection and rounding
restoration. Fresh Release builds use warnings as errors; sanitizer checks
include statistics, production, unit initialization/binding and construction
simulation. All 24 CPU/CLI tests, all 233 Python regressions and five affected
ASan/UBSan checks passed. Two generated Release test binaries initially lacked
execute permission; repairing those local permissions allowed the full suite
to execute. Independent review found no blocking issue. Windows floating-point
behavior remains unvalidated.

## Remaining live-unit boundary

Next is the complete spawn operation: recover jitter/position inputs, allocator
`2058`, model-slot cell records, output-bank commits and producer failure
handling. Registration is now available for that operation, but the construction
simulation still stores typed quotas and does not spawn live units. Ground
movement, aircraft binding, combat/AI and mission rules remain outstanding.

The capability estimate is **38% toward a playable single-player mission**,
up from 37%. One point credits verified registration and its quota bridge.
No credit is added for live spawning, movement or combat.
