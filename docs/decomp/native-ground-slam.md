# Original kind-44 hop and area damage

Continues PR #47 at `92d4a76`. With native ground transport and contacts enabled,
kind 44 now hops using its original timer, random direction and velocity rules.
Strong upward landing/contact impulses emit the original short-range area
health effect. No `health-delta` or readiness override is needed. Run:

```
comet_construction_replay < decomp/examples/ground-slam.scenario
```

The scenario produces both units through factories, then verifies automatic
hop/impact damage to a stationary enemy fighter. Its resources, construction
values and explicit enemy-base availability are development inputs. This is
one recovered damage producer, not complete weapon targeting or a campaign.

## Original evidence

The canonical ELF SHA-256 remains
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
FE5D8 job SHA-256 is
`9aca1695e6409668d9627ce865ad8008119f1c9e42d81a6106369f5a95e1872f`.
Addresses below are original job addresses unless marked PPU.

Kind 44 selects `17260` through `15BF8/15BFC`. The recovered update:

- Decrements entity `+EC` by elapsed time. Strictly below zero, adds four once.
  A hop consumes one shuffled-LCG draw through `1F2F8` to produce the original
  XZ sine/cosine direction; it does not use the two-draw spherical contact jitter.
- When flags `+60 & 2`, overrides position Y to float bits `BDCCCCCC` and uses
  spread `3D23D70A`; otherwise spread is `3DCCCCCC`. Lookahead steering comes
  from `13998` at modified position plus current steering. The normalized blend
  is current steering plus half the lookahead.
- Adds `1.4 * (blended direction + circle * spread) + (0, 1.1, 0)` to velocity,
  retaining the original float constants, FMA ordering and random-state update.
- Uses the grounded test `38D1B717 > ((positionY + 3E4CCCCC) - radius)`.
  External acceleration is multiplied by `3F4CCCCD`. Grounded non-hop motion
  adds `-5 * velocity`; descending airborne non-hop motion adds Y acceleration
  `-6`. The original low-alignment airborne steering bias, velocity integration
  and horizontal speed cap are retained. Zero velocity skips that steering bias.

The default external vector `(0, -1.5, 0, 0)` is recovered: PPU `CA9D8..CAA0C`
copies TOC `241DB8-70CC` (slot `23ACEC`, target `1BCD30`) into root `+3110`;
SPU `9DDC/9E04` copies snapshot `+2710` into context `+4C0`. The native kind-44
adapter supplies that original default; other mutable world-vector settings
have not been recovered.

`172D0..17374` fires when the complete current contact impulse has original
refined length strictly greater than one and refined normalized Y strictly
above bits `3F666666`. The gate is independent of the hop timer. Original visual
callbacks at `17378..17484` remain excluded. `17488..174EC` calls area routine
**`8AF0`** (not the preceding function tail `8AD8`) with stage mask 32, enemy-owner
mask, radius bits `3F199999`, damage 5, impulse strength zero, exponent 3 and mode 1.

PPU `CF430..CF498` constructs the enemy mask at root `+3130 + owner*4` from
player indices whose teams differ. Native routes supply those same team inputs.
The later extra-slot bit at `CF558..CF57C` has unproved semantics outside the
native four-player admission and is not synthesized.

## Area kernel and native gather

`arena_area_effect` exposes the original cell broad phase and per-entry cubic
kernel. The collector visits the current cell's bounded 3x3 neighborhood with
X outermost, Z innermost, preserving each ordered packed list. Original job
snapshot/gather scheduling is not reproduced.

Packed entry bytes 0..1 give the victim index, byte 2 the radius in hundredths,
byte 3 the marker, bytes 4..6 the quantized position, and byte 7 the owner/stage.
Position reconstruction uses the original FMA from cell origin `(x-.5,-.5,z-.5)`.
Both stage and owner masks must match. Mode 1 excludes marker 2, including
scouts (model slot 2); this filter is intentional. There is no extra self-ID gate.

Cell-center XZ distance must be strictly less than radius plus bits `3F350529`.
Per-entry XYZ distance minus effect and victim radii must be strictly below
bits `3C23D70A`. Signed health output is:

`-(1 - original_pow(max(distance - victim_radius, 0) / radius, 3)) * damage`

The original logarithm/exponential polynomial and reciprocal/root refinements
are retained. Replacing them with `x*x*x` changes output words. Falloff is not
clamped: the narrow accepted outer shell can yield a small positive health
change. The pure kernel also recovers radial impulse; the kind-44 caller's
strength is zero, so only health is integrated into the live accumulator.
Attribution, stun/effect metadata and original particle/visual notifications
remain unimplemented. This does not claim full stun behavior.

The contact accumulator now threads from zero through peer contacts and
terrain/bounds. Transport publishes it only after a successful world commit.
The simulation evaluates the event after motion/hop and before death, then
stages all victim health deltas transactionally. Existing native unit-index
order means later units consume effects this advance and earlier units on the
next advance. Rejected advances preserve RNG, timers, banks, cells and pending
health. Checkpoints replay the same hops, emissions, health and storage.

## Admission and limitations

- Kind-44 integration requires the existing successful routed transport path.
  `NoTarget`, `Blocked` and rejected movement retain the preview freeze policy;
  its hop/impact processing does not advance independently like converter pulses.
- Positive `+8C` cooldown can alter original speed-limit registers before this
  leaf. Supported native constructors initialize it to zero and no native
  producer currently sets it positive. Full cooldown/stun motion remains open.
- Helpers use round-toward-zero and restore caller rounding. Hop inputs require
  elapsed `(0,.25]`, timer `[-.25,4]`, positive normal speed limit, finite vector
  lanes of magnitude at most `1e10` and W zero. A zero blended hop direction
  rejects. Area geometry requires finite normal/zero XYZ within +/-64, radius
  `[.0001,64]`, 24x24 cell coordinates, and finite nonnegative magnitudes.
- Native exact-zero squared distance uses its geometric limit. The private
  interpreter's zero-exponent `FRSQEST` estimate is NaN; exact-zero distance
  fixtures are excluded from bitwise oracle claims. Hardware denormal and
  extended-exponent parity is not claimed.
- Projectiles, automatic firing/target selection for other weapons, enemy AI,
  aircraft, moving craft/controller input, original mission objectives and
  campaign progression remain unfinished.

## Validation

Untouched original instructions, using the existing interpreter revision
`a2c20108af45174dbb7d3e2f87f6acaaef09c6b4`, match:

| Comparison | Fixtures | Result |
| --- | ---: | --- |
| Impact trigger, including threshold float neighbors | 20,084 | All classifications equal |
| Hop/dynamics, including next RNG and Y override | 10,000 | All 120,000 output words equal |
| Single area candidates, masks/geometry/mode/impulse | 20,000 | Every emission word equal |
| Complete 3x3 neighborhoods | 1,000 | All 4,464 ordered emissions equal |

Hop comparisons supply the already-recovered route-leaf result at the `13998`
boundary; area comparisons intercept only `16F8` to capture emitted arguments.
Original hop/RNG/dynamics and complete area numeric instructions are untouched.
Original inputs, disassembly and oracle harnesses remain private.

Public tests cover numeric references, outer-shell healing, marker/team/stage
filters, invalid bounds, transactional multi-hit accumulation, contact output
rollback, hop RNG rollback under cell pressure, automatic hop/impact/damage,
checkpoint replay, and factory-to-damage CLI execution. All 233 Python tests,
48 headless / 49 graphics CTests, strict Release builds, compileall, repository
safety and eight focused ASan/UBSan tests pass (host leak detection disabled).
Original level-0 geometry/assets also load and render three frames with 40
models, 11 textures and ten complete framebuffers; that render smoke test does
not claim a new authentic campaign battle.
