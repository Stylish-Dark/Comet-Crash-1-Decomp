# Ground arrival at enemy bases

## Recovered behavior

Original job FE5D8, `6F40..7130`, scans four base entries. A live ground
unit qualifies when XYZ distance to `(cell.x+.5,0,cell.z+.5)` is strictly
less than `.5`, the base has an entity, its disable byte is zero, and its
team differs from the current unit team. The last eligible table entry wins.
The refined reciprocal-root arithmetic retains SPU round-toward-zero ordering.

`7290..735C` resolves the base's recipient statistics, calls `BF58` with
base-counter delta `-1` and attacking-owner attribution, emits a notification,
and marks the unit for removal. `6578..6724` unregisters its active count and
weight before changing it to a kind-63 tombstone. This path does not enter the
health-depleted combat-death effect branch.

`BF58..C170` is now a separate typed player-resource/statistics leaf:

| Decimal byte offset | Recovered update |
| --- | --- |
| 76 | add capacity delta |
| 72 | add resource delta, cap at updated 76 |
| 80 | add separate float delta |
| 84 | add unsigned count, cap at 10000 |
| 88 | enabled signed halfword delta, wrap then clamp negative result to zero |
| 120 | set attribution when old 88 was positive and enabled delta is negative |

The original global enable byte gates the counter delta and attribution.
The native caller supplies it explicitly; unsupported floats fail before writes.
Counter 88 is exposed as `base-counter`, rather than asserting unrecovered
mission lives/health terminology or inventing initialization and victory rules.

## Native owned-world integration

Configuration `base-counter OWNER VALUE` (0..32767, once per existing player)
opts that player's base into impact processing. `unit-base OWNER 1` also needs
to make the base available. Absent counter configuration preserves earlier
scenarios. Native base entries map directly to configured players; the original
indirect base/team/player lookup remains a bootstrap boundary.

After each transport substep the simulation uses the current owner/team, applies
base accounting and original unit unregistration, then removes the unit from
both ordered cell lists, decrements the cell summary and returns its index to
the FIFO pool. This corresponds to logical pool return `29F8..2AD0`; synchronous
owned storage clears freed slots immediately. Original kind-63 tombstone creation,
three-tick asynchronous delay, messages, effects and job scheduling are excluded.

Storage removal validates banks, unique membership and pool state before writes.
Movement, impact statistics, RNG and storage are staged together. Tick records
identify the base, current owner and retired index. The scene removes those
projections and remaps structure-cell links before projecting construction,
including same-tick reuse of retired indices.

Run the public factory-to-impact-to-production scenario:

```
cmake -S decomp -B build-decomp
cmake --build build-decomp
build-decomp/comet_construction_replay < decomp/examples/ground-base-impact.scenario
ctest --test-dir build-decomp --output-on-failure
```

Replay reports `base-impact` events and accepts `expect-base-counter OWNER VALUE`.
Counter zero stays zero; no mission win/loss transition or base disable is inferred.

## Evidence and boundaries

Private original-job SHA-256:
`9aca1695e6409668d9627ce865ad8008119f1c9e42d81a6106369f5a95e1872f`.
Oracle interpreter checkout: `a2c20108af45174dbb7d3e2f87f6acaaef09c6b4`.

10,000 deterministic arrival queries match original instruction execution (953
arrivals). Another 10,000 resource/counter queries match all 1,280,000 output
bytes, including enable gates, signed wrap, attribution and saturation. This is
leaf comparison, not whole-6F40 execution or hardware parity. Exact-zero and
subnormal squared distances cannot use the interpreter's extended-exponent
estimate as native IEEE data; native near-center handling selects distance zero
and is tested separately. Native position inputs are finite and bounded to 64;
base grid coordinates are bounded to 24. Invalid selector input returns no hit.

Public tests cover strict radius, vertical distance, team/availability, last-match
selection, accounting clamp/cap, FIFO reuse, malformed and duplicate retirement,
scene-link remapping and same-tick reuse. The replay checks real factory
reservations, route travel, quota release and renewed production. Independent
review found no blocking issue; capture-followed-by-impact and explicit
simulation-level failure rollback remain additional integration coverage gaps.

Validation: 35 headless CTests, 36 graphics-enabled CTests, 233 Python tests,
compileall, repository safety and ASan/UBSan (host leak inspection disabled).
Original level-0 assets also exercise explicitly placed base impact and rendering;
this does not establish autonomous mission behavior on that map. All original
assets, disassembly and comparison harnesses remain private.

## Remaining work and estimate

Automatic producer placement, original capture activation, weapon/projectile
and damage behavior, health-depleted corpses, special units, original neighbor
scheduling, enemy AI, authentic economy/HUD/audio, and mission win/loss/restart
remain. The native synchronous lifecycle and exact-center guard are integration
boundaries, not recovered original timing.

Estimated **64% toward a playable single-player mission**, up from 61%: a
capability judgment, not measured source coverage. Base impacts now connect
movement to an opposing-base counter and reusable capacity; this adds no claim
of completed combat or mission victory.
