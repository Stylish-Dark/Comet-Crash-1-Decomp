# Original factory timer and pending-unit reservations

This milestone builds on `b3ffd74`. Native factories now run the recovered
production timer and reserve pending units against player capacity. Reservations
are orders, not live entity spawns. Allocation and unit initialization follow
separate original paths. Their
verified native boundaries and remaining work are recorded in
[native-unit-templates.md](native-unit-templates.md).

## Recovered original boundary

The FE5D8 dispatch initializer at `A5F8..A8C8` sends types 26 and 28 to `10238`.
That handler calls construction update `A270` at `1025C`, then production `F5B8`
at `1026C`. Production receives the same elapsed interval, including the tick
where construction becomes ready. The native simulation follows that order,
updates both entity banks, and processes new construction/events afterward.

The port covers timer logic in `F5B8..F858` and quota reservation in
`C238..C378`. It intentionally omits subsequent preview/animation and effects.

| Original field | Meaning |
| --- | --- |
| Entity `+80` | Elapsed production timer |
| Entity `+84` | Production period |
| Entity `+AA` | Selected mode 0..4 |
| Entity `+B8` | Base unit kind: 40 for type 26, 44 for type 28 |
| Entity `+60`, bit 24 | Construction ready flag |
| Root `+10452` | Global production enabled flag |
| Player quota `+90` decimal | Signed active-unit count |
| Player quota `+94` decimal | Pending-unit count |
| Player quota `+96 + 2*(kind-40)` decimal | Pending count by unit kind |
| Player quota `+124` decimal | Unit capacity |

The original quota block is 128 bytes per player, located through root `+10240`.
The recovered period table at `20B80` contains:

| Unit kind | 40 | 41 | 42 | 43 | 44 | 45 | 46 | 47 | 48 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Period | 3.5 | 4.5 | 7 | 7 | 20 | 25 | 20 | 30 | 30 |

Production clears the timer when disabled or unready. Otherwise it adds elapsed
using SPU round-toward-zero arithmetic and denormal flushing. The reservation
threshold follows the original ordered operations:
`((period - 1) - float_bits(0x3e999999)) + 1`.
Crossing that threshold reserves one unit if `active + pending + 1 <= capacity`.
Success preserves the incremented timer; failure pins it at the threshold so
production retries next update. Exceeding the period resets the timer to zero.
Even a large interval reserves at most one unit per factory per update.

Unsupported kinds/modes/base-kind pairs and nonfinite or negative timer inputs
leave the leaf state unchanged. Invalid elapsed input throws. The caller's
rounding mode is restored. Native simulation quotas additionally require valid
nonnegative active counts, consistent pending totals and sufficient capacity;
the pure reservation leaf retains the original halfword counter behavior.

## Integration and reproduction

Configuration adds `production 0|1` and `unit-quota owner active capacity`.
Production defaults to disabled, with zero capacity. These explicit inputs avoid
inventing the title's still-unrecovered quota initialization. The scene exposes
actual timer/period fields; the viewer title shows these and the owner's pending
count. Existing F5..F9 controls select modes. Replay adds
`expect unit-pending0 N` (owner suffix) and `expect-unit owner kind N`.

```sh
build/native/comet_construction_replay --map /path/to/extracted/data --level 0 < decomp/examples/production-level0.scenario
```

The original level-0 scenario uses synthetic funds/tuning and explicit quotas.
At clock 22 it verifies ten ready/allocated structures, player-0 balance 280,
and four pending units: three kind 40 and one kind 44. This exercises the
imported factory, a manual factory with scheduled mode selection, and the
original map event's factory. Allocated structures remain ten: pending units
are not fabricated live entities.

## Validation

- Red/green leaf test, followed by public timer, threshold, full-capacity retry,
  disabled/unready, large-interval, rounding restoration and invalid-input checks.
- Simulation checks both banks, readiness ordering, quotas, funds and unchanged
  entity allocation. Replay checks configuration rejection and pending counts.
- A private original-instruction interpreter executes untouched `F5B8..F858`
  and `C238..C378`, including synchronous quota DMA. All 5,000 randomized
  fixtures (seed 126238) match the eight timer/period bytes and complete 128-byte
  quota block. Both factory types, all modes, owners 0..3, threshold crossings,
  full capacity, enabled/ready flags, overshoots and halfword wrap are included.
  This does not claim complete entity-byte or audiovisual equivalence.
- Full Release CPU/CLI suite: 21 checks; Python regression suite: 233 tests.
- Affected construction/state/storage/production tests under ASan and UBSan.
- Offscreen SDL/OpenGL rendering and injected build/tick/mode controls verify
  a pending reservation and refreshed timer/period fields.
- Assertion-based original level-0 production scenario described above.

Original binaries, disassembly, fixtures and private interpreters remain
untracked. The interpreter is ps3recomp commit
`5244db92669b54831c6d3ebb7a96d58c4dbfa6df`.

The unit initializer prefix and quota claim are now recovered; see
[native-unit-templates.md](native-unit-templates.md). Next: recover the remaining
binding/placement and allocation/failure path before connecting live spawns.
Movement, targeting, weapons/damage, enemy behavior and mission completion still
follow. This factory timing milestone moved the capability estimate to 33%;
the subsequent unit-template milestone adds two points, for 35%. These remain
planning estimates toward a playable mission, not a completed unit loop.
