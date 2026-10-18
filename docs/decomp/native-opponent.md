# Native preview opponent

Continues PR #50 (`55b6591`). An opt-in opponent now drives the existing
construction, factory production and ground deployment controls, so a native
preview can contain an active opposing side. This policy is new native code;
it is **not recovered original AI or original campaign balance**.

## Use

Add `opponent 1` before `run` to control player index 1. Each configured owner
must be active and have `production 1`, `transport 1`, `unit-base OWNER 1`,
a nonzero production quota, and kind-26 tuning. Do not also supply a `deploy`
control for that owner. The constructor enforces these dependencies for both
text and programmatic configurations. Flags default off.

A public synthetic encounter requires no game assets:

```
comet_construction_replay < decomp/examples/native-opponent.scenario
```

The opponent spends 60 resources on one ordinary factory, produces default
fighters within its quota, releases them from its base, and depletes the opposing
base through the existing movement and impact paths. The other player starts
with no defenses in this demonstration. The scenario asserts both final counters.

The same option works with the viewer's existing `--construction-config` input;
its player bases/active players must match the loaded map. P runs/pauses, B builds,
F5..F9 select a factory mode, held D releases units, and F2 restores the launch
checkpoint. The supplied replay is synthetic and does not claim a new original
asset/render validation or a complete original campaign mission.

## Policy and simulation boundary

- At most one decision per owner per simulation second, at the end of a positive
  advance. No wall clock, new random generator or background task is used.
- If an owned ordinary factory exists, select its default mode (selector 4),
  without resetting an already-selected timer. Otherwise consider non-base cells
  by squared distance from the owner's base, then Z/X order. Occupied/full cells
  are skipped; insufficient resources, allocation capacity and queue pressure wait.
- Construction uses the ordinary enqueue/step path: occupancy, funds, pool,
  shared queue and recovered route validation all apply. Rejected placement
  advances the candidate cursor and never debits funds. Global queued work and
  explicitly scheduled map commands retain priority. A busy global queue can
  delay an opponent; multi-owner decisions use player-index order.
- Production and release stop when the owner's counter reaches zero or there
  are no surviving hostile bases. Absent counters mean a preview base remains
  alive. Production precedes motion/impacts in the existing tick order; a quota
  reservation made in the same tick as elimination can remain pending.
- Automatic releases and movement rebinds use current staged base counters,
  inactive flags and teams to exclude dead/inactive/allied targets, including stale
  overrides. This filtering applies only to native opponents. Existing units use
  the recovered rebind path to choose another available enemy. Higher-level
  adaptive strategy and cancellation of previously queued map commands remain open.
- Scene projection and viewer/replay use the same authoritative simulation.
  Decision deadlines and candidate cursors are copied with the simulation, so
  F2 and arbitrary scene checkpoints include opponent state. Rejected time
  advances do not run decisions. NativeMissionPlayback freezes finished matches;
  the lower-level replay still permits explicit later advances for inspection.

## Validation

The opponent integration test exercises uncommanded construction -> production
-> deployment -> base depletion, ordinary resource debit, quota bounds, disabled
mode, poverty, exhausted allocation, occupied cells, friendly/eliminated owners,
existing factories after defeat, three-team dead/inactive target rejection with
forced overrides, invalid configs, rejected time, finished-match freeze and
restart. A chokepoint test verifies route rejection without debit/allocation,
then successful alternate placement and deterministic cursor restoration.
The replay test runs the public encounter and rejects duplicate/out-of-range
owners and missing production/transport/base/quota prerequisites.

Local validation: 59 headless CTests, 233 Python tests through both unittest and
pytest, compileall and repository safety. Focused ASan/UBSan checks cover opponent
and mission restart (host leak detection disabled). No new original-instruction
oracle claim is made: this milestone orchestrates already recovered controls.
Graphics and Windows checks are delegated to the repository's existing CI.

Remaining: original enemy decision logic, adaptive strategy, more unit/weapon
families, player craft/aircraft, authentic economy/HUD/audio and original
mission/campaign rules. Estimate **86% toward a first playable single-player
mission** (84% previously), using the prior capability yardstick, not measured
source coverage or completion of the full port.
