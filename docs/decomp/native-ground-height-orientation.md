# Original ground height and gradual orientation

Continues `0cb4995` / PR #39. Ordinary ground units in dynamics mode now turn
through the original quaternion interpolation rather than snapping to a route
heading. Contact mode also uses the original ordinary-caller height envelope and
its gated height spring. These are native C++ semantic kernels; original assets,
instructions, disassembly and comparison harnesses remain private.

## Recovered behavior

| Boundary | Native behavior |
| --- | --- |
| `14060`, `141D4..14210` | Deployed stage-5 caller with tuning halfword +52 equal FFFF and byte +56 unequal 1 selects local terrain, floor bits BE4CCCCC and ceiling bits 3F7FFFFF |
| Alternate `14528` / `7DD0` | World-only bounds, floor 0 and ceiling 2; linked steering remains outside this milestone |
| Context +595 / `14374..1441C` | Deployed steering with current-cell obstruction enabled (either bounds envelope): add four times `(1 - Y - velocityY)` to steering Y, then normalize XYZ |
| `15C84..15F90`, helpers `2400`, `2590`, `2250` | Direction quaternion, projected-up bank adjustment, shortest-arc quaternion interpolation, final normalization |
| `1711C` slow branch | Pre-limit horizontal speed squared <= bits 3E4CCCCC selects prepared steering; otherwise select capped velocity |
| `159E0`, `15A88` | Interpolation factor = elapsed * bits 412FEDDE * entity +E8; bank scale from tuning +10 (native slot +90) |

The obstruction flag comes from the cached old cell and owning team's visibility
bit, suppressed by entity bit 0x80000. It enables height control after either bounds envelope in the
deployed steering branch. Terrain correction precedes steering preparation;
acceleration and speed limiting precede orientation. The prior quaternion comes
from the read bank. Invalid results leave both banks, cells, accounting and RNG
unchanged. Kind 46 retains the explicit preview route heading because its special
`17160` orientation branch is not recovered.

## Numeric scope and validation

Height arithmetic preserves round-toward-zero, separate subtraction and original
FMA/estimate/refinement order. A seeded private comparison of 10,000 original
`143A0..1441C` executions matches all 40,000 float lanes bit for bit.

Orientation recovers the original sine, inverse-root, reciprocal, atan and acos
polynomials and their interpolation thresholds. Of 6,000 original executions
(`15C84..15F94`, seed 7521), 4,216 four-lane quaternions are bit-exact;
all 24,000 lanes differ by at most 1.7881393432617188e-7 (acceptance 2e-7).
Inputs cover finite XYZ velocity, random unit priors, elapsed .0001..08 and bank
scale -1..1. This is tolerance-verified arithmetic, **not complete bitwise parity**.

The IEEE interpreter is nonfinite at the zero cross products encountered in flat
motion and cannot establish SPU extended-exponent parity there. Explicit native
zero-side/zero-swing bank guards use zero bank; a zero direction retains prior
orientation. Height rejects a zero/non-normal squared direction. These guards are
native policies. Public tests exercise flat turning, slow steering selection,
normalization, rounding-mode restoration and invalid inputs. Live tests cover
height envelope selection, obstruction-enabled upward acceleration, gradual
turning and synchronized banks.

Validation also covers all 34 native/CLI/graphics CTests, 233 Python tests,
ASan/UBSan (host leak inspection disabled), and original level-0 movement/rendering.

## Remaining scope and progress

Still required: original neighbor gather and scheduling, producer deployment,
base/linked and special-kind state, collision-driven lifecycle effects, hostile
capture, weapons/projectiles/damage, mission rules and enemy AI. Original kind
40/43 random-bounce branches are not included. No combat or completed-mission
credit is claimed. Dynamics/contact modes remain explicit preview controls.

Estimate: **58% toward a playable single-player mission**, up from 55%. This is a
judgment of capability and remaining integration, not measured source coverage or
a mechanical sum of the older construction rubric. Original hardware parity,
campaign completeness and multiplayer are separate goals.
