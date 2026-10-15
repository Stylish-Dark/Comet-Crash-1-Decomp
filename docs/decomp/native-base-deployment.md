# Selected base deployment

Factories reserve orders; the original base/player controller releases them.
This milestone connects that selected release path to the native world so a
configured held input can repeatedly deploy produced ground units without
manual `spawn` commands.

## Recovered source boundaries

- Job FE5D8 `CB04..CB10` adds elapsed time to controller timer `+0x0C`.
- `DA48..DA8C` reads selected kind index `+0x18`, looks up table `20BB0`,
  computes `fma((interval - 2), strength, 2)`, and requires timer strictly
  greater than that threshold. The native caller supplies the held-input
  strength explicitly and does not release with zero strength.
- `DABC` calls `BA08` to claim the selected pending kind. `DB80` calls
  `C380` with job-cell center `20DF0`. `DBE8` resets the release timer only
  after spawn succeeds. Missing orders and failed spawns do not reset it.
- `1250..1260` builds `20DF0 = (cell.x+.5, 0, cell.z+.5, 0)`.

| Produced kind | Full-strength interval |
| --- | --- |
| 40..43 | float bits `3D4CCCCC` (approximately .05 seconds) |
| 44, 46 | .5 seconds |
| 45, 47, 48 | 1 second |

The pure timing leaf covers the nine table entries. Native deployment supports
the existing seven ground kinds: 40, 41 and 43..47. Aircraft 42/48 are rejected
without consuming orders. Timing preserves round-toward-zero subtraction/addition
and fused threshold arithmetic; replacing the fast interval with rounded `.05f`
would change boundary decisions.

## Native controls and scheduling

`deploy OWNER KIND STRENGTH` before `run` configures one player (no duplicate
entries). The same command after `run` changes the runtime control. Strength is
finite 0..1; zero releases the held input while the available base's timer keeps
advancing. Default configuration has no deployment control, preserving previous
scenarios. A configured `unit-base OWNER 1` is required for release.

Native fixed-base position uses the configured player's base cell. These typed
controls replace the selected fields of the original base-controller input;
original gamepad decoding, moving player craft, controller health/game-phase
gates and autonomous AI selection are not claimed here. No priority scan of
pending kinds or factory-position spawn heuristic is introduced.

The native frame first advances construction/reservations and existing units,
then attempts at most one release per configured player. New output records begin
moving on the next tick, so a coarse elapsed interval does not backdate newborn
movement. This is synchronous scheduling, not the complete original C730 job.
Timer, storage, RNG and statistics use the existing staged owned-world commit;
configured deployment also uses the ten-second native elapsed-time bound.

Successful release uses the recovered constructor/jitter/binding, registration,
cell packing and FIFO allocation path, then resets its timer to zero (overshoot
is discarded). Allocation pressure retains the existing original claim and
compensating-unregistration semantics: the attempted order is consumed, active
accounting is compensated, and the timer stays ready. This is not a retry queue
that restores lost orders. No-order attempts leave actual RNG unchanged.

The scene removes retired projections before adding construction and newly
deployed units. Same-tick retirement/reuse therefore replaces the old unit
instead of displaying two objects with one pool index. Manual deployment still
uses its existing API and shares the unit projection helper.

## Runnable continuous loop

```
cmake -S decomp -B build-decomp
cmake --build build-decomp
build-decomp/comet_construction_replay < decomp/examples/base-deployment.scenario
ctest --test-dir build-decomp --output-on-failure
```

The example uses explicit synthetic economy and held input. A factory produces,
the base releases, the unit travels and hits an enemy base, and released quota
and pool capacity permit another production/release/impact. There are **no manual
spawn commands**. Replay reports `base-deployment` attempts with owner, kind,
status and allocated index. The same config can drive native viewer playback;
this milestone does not add a new keyboard/controller mapping.

## Evidence

Private original-job SHA-256:
`9aca1695e6409668d9627ce865ad8008119f1c9e42d81a6106369f5a95e1872f`.
Oracle interpreter: `a2c20108af45174dbb7d3e2f87f6acaaef09c6b4`.

20,000 deterministic original-instruction queries match timer words and release
readiness (8,436 ready). Another 54 exact-threshold/next-float checks match.
All 576 job-cell center vectors match all four original float words. These are
scoped arithmetic/control comparisons, not full-controller or hardware parity.
Finite normal/zero timing inputs and elapsed 0..10 seconds are the native domain;
invalid inputs reject without changing timer or ambient rounding mode.

Public tests cover success/no-order/allocation pressure, held-input changes,
newborn position and movement ordering, two deploying owners, legal full cells,
same-tick retirement/index reuse, bounded-clock rejection, and the continuous
factory-to-impact replay. Independent review found no blocking defect; its
coverage suggestions led to the multi-owner/capacity/reuse/clock cases.

Final local validation: 37 headless CTests, 38 clean graphics-enabled CTests,
233 Python tests, compileall, repository safety and ASan/UBSan (host leak
inspection disabled). Original level-0 factory production and selected base
release also run with user-owned assets: unit 9 is born near `(14.4314,1.55401)`,
moves on the next tick, and a second order releases another unit. The renderer
loads 40 models, 11 textures and 10 complete framebuffers and presents three
frames. This does not establish autonomous original level-0 mission behavior.
Original assets, executable/disassembly and oracle harnesses remain private.

## Remaining work and estimate

Original moving-craft input/deployment control, AI choice, aircraft, capture
activation, weapons/projectile damage, original neighbor/job/tombstone scheduling,
authentic economy/HUD/audio/effects, enemy behavior and mission win/loss/restart
remain incomplete. This is a functional selected base-release loop, not a
completed playable mission or full original producer controller.

Estimate: **69% toward a playable single-player mission**, up from 66%.
This credits the continuous production/deployment/movement/impact loop; it is
a capability estimate rather than measured source coverage.
