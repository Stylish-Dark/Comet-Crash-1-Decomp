# Original hostile ground-unit capture

Continues `166a638` / PR #40. Contact mode can now transfer an overlapping ground
unit to a hostile kind-45 unit's owner. Accounting, target state, bank/cell records
and displayed ownership follow the transfer. This is the first recovered hostile
ownership interaction in the native preview; weapons and mission outcomes remain
separate work.

## Original boundary

The pure `capture_arena_ground_unit` implements the ownership/accounting branch
of simulation-job `7E40`, specifically gates `8250..83F0`, admission/removal
`83F4..8454`, and ownership/link/target writes `8458..850C`. Pair distance,
penetration and mass share are supplied by the existing recovered pair kernel.

| Input or state | Required behavior |
| --- | --- |
| Pair geometry | Overlapping according to the recovered original negative-gap threshold |
| Victim | Deployed stage 5; supported ordinary ground kind 40, 41, 43, 44, 46 or 47 |
| Peer | Kind 45 in the supported ground state; readiness byte at entity +BB is nonzero |
| Teams | Different team identifiers; peer flag +60 bit 0x20000 suppresses hostile classification |
| Admission | Register victim with recipient, adding=true and count_active=true, before removing it from old owner |
| Capacity rejection | Keep original admission's partial statistics changes; retain victim ownership and source accounting |
| Success | Preserve lower five bits of victim +33; copy peer owner bits; write FFFF to victim +1E and copy peer target +7E |

The existing `48C8` / `BB30` semantic registration updates weighted ground counts
and active quotas. Capture consumes no production order and allocates no new
entity. Invalid native arithmetic or unsupported registration leaves the helper's
caller-owned state unchanged. Capture of a kind-45 victim instead clears its
health in the original; that lifecycle branch is deliberately excluded.

## Live integration and controls

`transport_arena_ground_unit` accepts optional raw player-statistics blocks.
Without them, its earlier friendly-only contact behavior remains. With them,
nearby hostile ground contacts use the recovered pair arithmetic and capture
branch. The native broad phase and synchronous scheduling are still policies,
not original gather order or SPU job batching.

The complete movement transaction works on copies. A blocked route, invalid
copied target, malformed statistics or failed movement preserves banks, cells,
summary counts, statistics and RNG. After transfer, terrain, steering and crossed
route checks use the new owner. Copied targets must be an in-range enemy other
than that owner; unsupported copied state rejects the complete native step.
The existing initial old-owner target binding still precedes contacts, so a
victim returning NoTarget at that point does not enter capture processing.

The simulation seeds movement statistics with that tick's production reservations
and reads updated quotas back after movement. This prevents capture counts from
being overwritten by the old production quota snapshot. Scene refresh now reads
a moving unit's owner as well as position/quaternion, so team display follows it.

Readiness is **explicit native preview control**, not recovered automatic weapon
activation. `set_ground_capture_ready(index, bool)` updates +BB in both banks of a
live stage-5 kind-45 record; invalid indices and other kinds are rejected. It
neither invents a cooldown nor enables all kind-45 units by default.

Replay commands:

- `capture-ready index 0|1` sets that explicit readiness state.
- `expect-unit-owner index owner` checks a live unit's raw ownership.

Run `decomp/examples/ground-capture.scenario` through `comet_construction_replay`.
It builds two factories, reserves kinds 40 and 45, spawns them, enables readiness,
then verifies capture, player counts, unchanged allocation count and no duplicate
transfer on a subsequent tick. Tuning/resources and placement are synthetic.

## Validation

A private original-instruction harness executes `8250..8510`; it stops before
notification/visual callbacks and provides synchronous successful atomic-statistics
loads/stores. Original instructions are untouched. With seed 7848458, 10,000
cases match every output byte of the victim (256) and four statistics blocks
(512), plus observed capture classification: **7,680,000 bytes and 10,000 capture observations**.
There are 396 successful transfers; cases vary ground kinds, all four owners,
teams, readiness, immunity, quota pressure, weights and preserved raw bytes.
The harness omits original atomic contention and effect callbacks.

Public tests cover eligibility gates, exact ownership/link/target writes, quota
transfer, rejection, invalid arithmetic, scene ownership, repeat ticks, readiness
validation, factory-to-capture replay, and complete rollback after a late route
or copied-target rejection. Review found and fixed statistics-span overflow and
copied-target validation defects; five-player spans and invalid/self targets are
now regression cases. No unresolved review blocker remains.

All 35 native/CLI/graphics CTests, 233 Python tests and ASan/UBSan checks pass
(host leak inspection disabled). Original level-0 capture and model rendering are
also checked privately. No original assets, executable data or harnesses are added
to the repository.

## Remaining work and estimate

Original capture-readiness activation, kind-45 victim death/lifecycle,
notifications and visual effects, original neighbor gathering/scheduling,
automatic producer placement, base/special state, weapons/projectiles/damage,
enemy AI and mission win/loss remain outstanding. This is not a completed combat
system or playable mission.

Estimated overall progress: **61% toward a playable single-player mission**, up
from 58%. This is a capability judgment, not measured source coverage. It credits
the recovered hostile transfer and owned-world integration; no weapons/damage or
mission-completion credit is added.
