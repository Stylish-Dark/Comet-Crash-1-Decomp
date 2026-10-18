# Native cannon projectiles and kinetic impacts

Continues the Tesla milestone at `b3c1a27` (PR #49). Completed the retained local
cannon recovery work and connected a ready kind-21 tower to native kind-3 rounds,
designated ground-target collisions, deferred impulses and pool retirement.
Enable `cannon 1` in a construction configuration. It defaults off and enables
velocity dynamics at bootstrap, so movement preserves the rounds' knockback.
Ground transport must also be enabled. Run the public encounter with:

```
comet_construction_replay < decomp/examples/cannon-combat.scenario
```

The scenario constructs a cannon and enemy factory, produces a fighter, then
checks firing, a kinetic impact, a surviving round and unchanged fighter health.
The scene projects model 27 rounds at their live position/orientation and updates
the cannon's source yaw. Original composite barrel rendering remains separate.

## Recovered boundaries

Evidence is the same exact-title ELF and FE5D8 job pinned in
`native-tesla-combat.md`; addresses below are FE5D8 job addresses.

- Kind-21 wrapper `1D1B0` invokes source `1CF88` / common targeting `1B818`.
  Native source retains target-cache `+1E`, firing timer `+BC`, refresh timer
  `+C0`, yaw/pitch `+C4/+C8`, turn rates `+D4/+D8`, pitch limits `+E8/+EC`,
  projectile speed byte `+CD` and angular tolerance byte `+DD`.
- `1EEC8` supplies the moving-target interception arithmetic. The aim helper
  retains original angle reduction, polynomial trigonometry, turn limits,
  quaternions and angular firing gate. It uses finite analytic limits for zero
  direction and unit-vector rounding edges where original math can be nonfinite.
- Constructor `1D028..1D17C` creates kind 3/model 27/stage 1, quantity 6,
  mass scalar 1 and health/max-health 50. It stores parent index at `+6C`,
  initializes count `+80=0`, word `+84=45`, and launches toward the predicted
  intercept. The fire timer resets before allocation, including failed allocation.
- Source gathering uses the already recovered `4428/4078` local-first priority,
  restricted here to ordinary stage-5 ground units. Cache validation admits only
  opposing teams. Native predicted-range rejection drops and reacquires a stale
  cached target rather than aborting the tick.
- Kind-3 update `AE58` invokes bounds `7A50` before velocity integration; it adds
  world acceleration while Y is above 1 and updates its flight quaternion/count.
  `AF18` saves the bounds return, and `B018..B048` combine it with the pair hit
  to select retirement. Native bounds use X/Z `[-2, dimension+2]`, floor raw
  `BE4CCCCC`, ceiling 8 and the entity's movable/floor-radius flags.
- The original `B00C -> 7E40` collision checks the designated `+1C` target;
  this is not an incidental nearest-target or swept-ray gather. Missing/retired
  target and parent-index exclusions consume no target jitter. An admitted
  target consumes two original random-direction draws even without overlap.
  Existing pair preparation supplies predicted-position geometry, index-directed
  coincidence correction and the strict negative-gap overlap gate. Team equality
  does not suppress an already-linked projectile collision.
- `B080..B0C4` emits velocity times projectile mass as impulse and negative zero
  as the direct health delta. **This path does not inflict direct health damage.**
  The original impulse consumer limits vector magnitude to 8, applies reciprocal
  victim mass and restitution, and adds velocity only when flag `0x4000` permits.

## Native scheduling and limits

The caller stages banks, cells, FIFO, RNG and pending impulses together. Each
cannon pass visits ready sources and then rounds in index order, using substeps
no longer than .05 seconds. Ground movement precedes weapon passes; accumulated
impulses are consumed once on the next ground advance. Tesla passes precede
cannon passes. New rounds may advance during their creation pass. These are
explicit native scheduling choices, not original asynchronous SPU timing.

Missed rounds retire after 600 projectile updates. They also retire before
exceeding native storage's +/-64 position domain, on recovered energetic bounds
contact, or when a destination packed cell is full. These capacity/domain/age
policies keep preview battles running and preserve the finite pool; the age cap
is not a recovered original lifetime. A full source pool simply prevents allocation.
The adapter uses fixed original default world acceleration `{0,-9.8,0,0}`.

Source terrain observer `88F0`, visibility/occlusion callbacks, structures and
aircraft as targets, successful particle/sound allocation, composite weapon parts,
other towers and original concurrent output scheduling remain unimplemented.
Native synchronous retirement can reuse an entity index while an old projectile
still links that index; there is no allocation-generation identity here. Original
link/team checks are retained, but complete original tombstone timing is not claimed.
The generic projectile helper also has a tested kind-15 boundary; the live adapter
only creates and advances kind 3.

## Validation

- 58 headless checks / 59 checks with SDL/OpenGL, including the public firing/impact
  scenario, bank/cell/pool consistency, disabled/allied sources, exhausted source
  pool, full destination cell, downward/high-speed misses and scene projection.
- Explicit restart checkpoints taken both with an in-flight round and immediately
  after an impact (pending impulse); replay restores banks, cell lists and FIFO.
  Motion-off and rejected-clock checks preserve combat state.
- 233 Python tests, compileall, repository safety and eight focused ASan/UBSan
  checks pass. Leak detection is disabled for the host tracing environment.
- Original level-0 asset smoke renders three frames with 40 model entries,
  28,890 vertices, 10,202 triangles, 11 textures and ten complete framebuffers.
  This smoke is asset/render integration, not a complete original battle.
- Fresh private instruction comparisons: 10,000 complete projectile constructors,
  10,000 complete impulse-consumer slots, 9,998 complete finite source outputs
  from 10,000 fixtures, and 10,000 aim plus 10,000 lead fixtures comparing every
  finite original output lane. Two source cases and some aim lanes produce
  original nonfinite values; native analytic limits are deliberately distinct.
- The separate full projectile-update instruction comparator could not finish:
  its private interpreter aborts on unsupported `mpyui` at LS `0x1398`. Full
  instruction parity for that update is not claimed. Public projectile tests cover
  integration, impact emission, gravity, orientation, rejection and byte preservation.
- Independent review caught and reproduced full-cell and Y-escape tick aborts;
  public regressions reproduce both failures and pass with explicit shot retirement.

Original data, disassembly and private validation harnesses are excluded from Git.
Remaining playability blockers include autonomous enemy decisions, other weapons,
craft/aircraft controls and original mission/campaign objectives. Capability
estimate: **84% toward a first playable single-player mission**, previously 82%;
this is judgment, not measured source coverage or full-port completion.
