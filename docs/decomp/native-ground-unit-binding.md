# Complete original ground-unit constructors

This milestone builds on `b2ce6d2`. The native port now completes the original
`14B30` constructor for the seven stage-5 ground-unit kinds: 40, 41 and 43..47.
It selects a target base, checks the unit's route, and initializes the original
orientation and velocity. Constructor return codes are preserved separately from
model slots (kinds 40..47 return 47..54; kind 48 also returns 54). Aircraft kinds 42 and 48 retain their verified prefix;
their continuous-direction initialization remains outside this boundary.

These are constructor and inspection operations. They do not allocate a live
unit, advance its position, execute combat, or consume production orders.

## Target selection

The constructor calls `12BE0` at `14FD8`. It reads the unit owner from packed
byte `+33`, then reads that player's override byte from root
`+9504 + 64*owner + 94` (decimal offsets). A non-255 override whose base index
at root `+9984 + 4*target + 112` is not -1 is accepted immediately. This branch
bypasses team filtering and consumes no additional RNG value.

Otherwise, `3068` calls `2CE0` to collect available enemy bases in player order.
Player count is root `+10272`, and team bytes are root `+9504 + 64*player + 88`.
Candidates must have a different team from the unit owner and a base index other
than -1. With candidates, one original RNG draw selects `draw % candidate_count`,
even when there is only one candidate. Without candidates, no selection draw is
consumed. Target byte `+7E` becomes the selected player or 255 if none exists.

## Route gate and initial motion

After binding, the constructor reads signed-halfword cached x/z from `+C8/+CA`,
selects that cell's directed route byte through `1FB20`, and computes the pair
index as `3*owner + target - (target > owner ? 1 : 0)`.

- No target: return with the initialized template and `+7E=255`.
- Route bit 0 set: retain the template's orientation and velocity; target
  assignment remains in place.
- Route bit 0 clear: direction bits 1..3 select one of the eight fixed vectors
  at `20AF0`. The ground branch at `1589C` calls `2400` with the negated vector,
  writes its quaternion to `+20..+2C`, and writes the vector multiplied by unit
  scalar `+84` to `+40..+4C`.

The port uses the exact quaternion results for those eight fixed inputs,
including signed zero and original approximation residuals. It does not replace
this finite lookup with host trigonometry. Velocity multiplication retains
round-toward-zero arithmetic and denormal flushing; the caller's mode is restored.

Native guards reject aircraft, non-stage-5 templates, absent/out-of-range owners,
cached coordinates outside 24x24, and nonnormal/nonpositive scalar `+84` before
writing bytes or drawing RNG. They also reject out-of-range or self-target
native overrides: self-targets do not name a valid directed route pair. This is
a native safety boundary, not a claim that the original override branch checked
for self-targets. Valid original constructor inputs are compared below.

## Owned simulation and reproduction

`factory_ground_unit_template(index)` initializes an inspection unit at the
factory position and completes the ground binding operation. It shares one copy
of the simulation RNG between prefix initialization and target selection. Repeated
inspection preserves future simulation randomness, entity banks, quotas, funds,
clock and allocation. This placement is an inspection policy, not recovery of
spawn positions or factory preview animation.

Header configuration adds explicit world inputs:

| Directive | Meaning |
| --- | --- |
| `unit-base owner 0_or_1` | Whether that player's original base index is available |
| `unit-target owner target_or_255` | Override target; 255 means normal selection |

Availability defaults to false, and overrides to 255. Availability is separate
from construction route inactivity; it is not inferred from player resources,
map rendering or unrecovered base health. Teams and routing bytes come from the
existing configured simulation. Duplicate declarations and malformed owners,
overrides or flags fail with a line-numbered error.

Replay adds `expect-factory-ground-unit index target status`, where status is
`no-target`, `blocked` or `bound`. Missing factories and aircraft fail the
expectation. The existing prefix-only inspection remains available for all
nine kinds.

```sh
build/native/comet_construction_replay < decomp/examples/ground-unit-binding.scenario
build/native/comet_construction_replay --map /path/to/extracted/data --level 0 < decomp/examples/ground-unit-binding-level0.scenario
```

The synthetic scenario checks ground selections from both factories, blocked
routes at occupied factory cells, unchanged funds (190) and allocated structures
(two), followed by a pending production order. The level-0 scenario checks
bound units from three factories with explicit base availability and the existing
configured route fields. It retains ten allocated structures, four pending unit
orders, and player-0 balance 280. Its tuning, funds and world inputs are synthetic;
this is integration validation, not authentic title balance.

## Corrected remaining allocation boundary

`48C8` was previously described as placement validation. Its recovered body
actually builds a statistics delta and calls `BB30`, which updates a player
statistics block. Full registration/capacity/failure behavior remains unrecovered.
`C380` calls the unit constructor, this registration path, and allocator `2058`;
the surrounding producer's failure handling also remains to be ported. Completing
a unit constructor is therefore not sufficient to claim live spawning.

## Validation

- Red/green tests for ground binding, simulation inspection and replay support.
- Public original reference vectors for all eight headings and all seven ground
  kinds; exact quaternion/velocity words, directed pair/cached-cell selection,
  blocked-byte preservation, no-target/override RNG behavior, missing-base
  fallback, rounding restoration and invalid-input preservation.
- A private untouched-instruction interpreter runs the complete original
  `14B30` routine. All 5,000 complete 256-byte slots, constructor return codes and subsequent RNG outputs
  match native initialization plus binding (seed 151050). Fixtures vary reused
  bytes, kinds, owners, 1..4 players, teams, base availability, overrides, seeds,
  positions and route bytes independently across cells and directed pairs.
- Independent code review found no blocking correctness issue; its coverage
  suggestions were incorporated into the committed tests.
- Fresh Release build with warnings treated as errors: all 23 CPU/CLI checks.
- All 233 Python regressions and 11 affected sanitizer checks.
- Offscreen SDL/OpenGL rendering with injected build/tick/mode controls and
  verified ground-unit inspection on the constructed factory.
- Both assertion-based scenarios above.

Original executable bytes, disassembly, private interpreter and randomized
fixtures remain untracked. Committed reference vectors contain resulting numeric
state values, not original instructions. Windows behavior remains unvalidated.

The weighted capability estimate is now 37% toward a playable single-player
mission, up from 35%. Two points cover completed ground constructor target/
heading initialization; live allocation, movement and combat are still absent.
Next: recover statistics registration and the allocation/failure path, followed
by live-unit advancement. Aircraft binding and mission rules remain outstanding.
