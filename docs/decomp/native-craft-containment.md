# Original craft arena containment

Continues PR #55 (`02524f2`). Both of its regression and native-source-port
workflows passed, including Linux graphics and Windows build/package jobs.

The next bounded milestone replaces NativePilot's X/Z position clamp with the
original craft's six-plane world response in the original flight-update order.
This composes previously recovered contact arithmetic; it does not recover the
whole craft controller or its collision-shape initialization.

## Original evidence

The user-owned ELF SHA-256 remains
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
The FE5D8 job SHA-256 remains
`9aca1695e6409668d9627ce865ad8008119f1c9e42d81a6106369f5a95e1872f`.

| Instruction range | Recovered meaning |
| --- | --- |
| `10500..10544` | Integrate all position lanes with old velocity |
| `10548` | Call craft containment wrapper `7DD0` |
| `7DD0..7E2C` | Read unsigned arena dimensions from snapshot `+28D8/+28DC`; pass `(0,width,0,depth)`, floor 0, ceiling 2, and no optional sphere points to `7A50` |
| `7A50` | Check X-min, X-max, Z-min, Z-max, floor, ceiling in order using `7510` motion response |
| `105B8..105FC` | Read contact-adjusted velocity, respond toward desired velocity, clear new Y velocity |

The shared contact recovery documents the primary/lateral radius, movable and
floor-radius flags, restitution, separating-velocity gate, positional epsilon,
normal impulse and energetic-impact result in [native-ground-contact.md](native-ground-contact.md).
An energetic impact is motion metadata; it is not a damage event.

## Native implementation

`advance_arena_craft_in_arena` composes integrate -> contain -> respond.
`advance_arena_craft_flight` retains the prior collision-free API and its exact
numeric fixtures. State is copied and committed only after all stages validate.
Bad extents/radii, unsupported input, or output overflow throw without mutation;
rounding is restored on success and exception. The boundary result exposes
per-step accumulated impulse (initially zero), plane count and energetic status.
External lifecycle/effect callbacks remain suppressed.

F3 pilot movement now uses the composed path and retains contact-adjusted velocity
between frames. For example, an outward-moving craft released at a wall receives
the original normal impulse before damping. The old clamp merely clipped its
position while retaining outward speed. Original epsilon correction also puts
the craft just inside the wall rather than exactly on the clamp coordinate.

Preview choices remain explicit: primary and lateral radius .5, restitution 100,
movable enabled, alternate floor-radius flag off, source mode flag 1, reduced
speed off, initial Y .85, heading from X/Z velocity, and input-cancel velocity
reset. These shape/material values preserve the previous half-cell footprint;
they are not claimed to be original craft constructor values. The ship remains
a control/render object, outside the combat/economy entity registry.

## Validation

A private harness executes untouched original flight blocks and the complete
`7DD0`/`7A50`/`7510` motion path in pinned ps3recomp interpreter
`a2c20108af45174dbb7d3e2f87f6acaaef09c6b4`. It stops at external `1E880` effect
callbacks and resumes at their return addresses; no motion instructions change.
Seed **71956**, **10,000** queries: **120,000 float words and 10,000 boundary-hit
returns match exactly**, with **8,442** boundary-hit cases. Inputs vary XYZ
position/velocity, zero W, rectangular extents 2..24, distinct radii, restitution,
movable/floor flags, commands, delta and both response/reduced-speed branches.
Ordered big-endian 13-word output SHA-256:
`4c7de4493c7928cc16548051d89a674eac00543ae73a254b103f7c37dcc4573e`.
Twenty-four synthetic numeric fixtures are public; private original inputs and
oracle code are excluded from Git. Hardware denormal/extended-exponent parity is
not established; existing native subnormal policy remains.

Public tests pin the original-order wall impulse, all six planes, alternate floor
radius, invalid-input rollback and rounding restoration, diagonal wall holding,
escape from a wall, reset and the one-cell preview arena. The new pilot wall
assertion failed against the old clamp before implementation. Existing SDL virtual
pad tests still exercise flight-to-build at the east wall, pause and restart.

Verification results: **63 headless / 66 graphics-enabled CTests** (complete final
rerun green), **233 Python regressions**, and
**four focused ASan/UBSan CTests** passed (craft, pilot, contact and gamepad;
leak detection disabled for this host). Compileall, repository safety and diff
checks passed. Independent code review found no actionable issues. Graphics
coverage uses synthetic models and SDL virtual input, not a physical pad or a
fresh original-asset render. One initial graphics run failed the pre-existing
`comet_native_viewer_tests` keyboard-construction timing assertion; its unchanged
focused rerun passed. The test injects commands on fixed delays after SDL init,
so graphics startup delay can coalesce its manual ticks. No production fix or
claim of a resolved timing flake is made.

## Remaining work and estimate

Recover original craft size/material initialization, `3238` world query, terrain/
entity collisions, `+51 & 1C` state gates, attachment and action branches, angular
motion and carrying/combat/economy ownership. Aircraft, richer original AI,
mission/campaign rules, HUD, audio and save/presentation parity remain open.

Capability estimate: **92% toward the first playable single-player mission**,
from 91%. This is a subjective capability estimate, not source coverage or
full-port completion. The increase credits integrated original boundary motion;
it does not credit unrecovered craft actions or original campaign behavior.
