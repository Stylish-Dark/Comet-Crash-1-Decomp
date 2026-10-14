# Original ground terrain response

Continues `6a2ea7f` / PR #38. The native contact mode now uses the original local
wall rectangle and four corner points from simulation-job routine `20`, composed
with the recovered six-plane `7A50` response and motion-only `7970` point response.
A unit approaching a wall can advance tangentially while its normal velocity is
reflected; previously the native route-cell guard rejected the whole step.

## Recovered boundaries

| Routine | Behavior |
| --- | --- |
| `20..508` | Select a rectangle from cardinal obstruction bits, world dimensions, cached cell and team; prepare four corner points or select world-only bounds |
| `7970..7A4C` | Use point minus reference position, refined distance, lateral radius and the original negative epsilon; normalize with reciprocal/refinement and call motion response |
| `7A50` optional-point path | Six planes followed by NW, SW, NE and SE point responses, retaining accumulated impulse |
| `9D58..9EC0` flag producer | Current-cell obstruction selects world-only behavior unless entity bit `0x80000` suppresses that flag |

Walls use the owning team's low visibility bit. The original rectangle starts
at world bounds and narrows to the current cell boundary when a cardinal neighbor
is obstructed. A diagonal corner is retained when its diagonal cell is obstructed
or it borders the world edge. Adjacent cardinal walls do not suppress it. These
edge and diagonal details were established by original execution, rather than
substituting an intuitive geometric rule.

All four points are evaluated in original order, including zero placeholders.
They can respond near the world origin. They are not silently discarded as
inactive corners. Point penetration is tested against the cached reference
position, while correction updates the current position; those positions can
differ after integration. The native adapter supplies old X/Z with reference Y
and W zero, preserving packed entity-header bytes in live writes.

Arithmetic uses round-toward-zero, original estimate buckets and preserved FMA
order. The point routine returns whether a contact occurred; energetic impact
is retained separately from the shared response and is not credited as damage.
Semantic C++ runs directly; no original instruction interpreter is shipped.

## Live integration and scope

Terrain runs inside the existing opt-in `contacts 1` / viewer C mode after
position integration and before steering/velocity update. It uses the selected
unit's old cell and reference position. The local slot and copied RNG commit only
after route checks and both-bank list/cache/count migration succeed. Wall and
corner response do not mutate peer records or consume extra RNG.

The conservative crossed-route-cell guard still rejects unsafe long steps that
remain inside an obstruction after response. This is not continuous swept
collision. Native friendly neighbor gathering and synchronous update scheduling
remain preview policies. The adapter retains floor 0 and ceiling 2 from the
previous world containment envelope. The complete ground caller's alternate
floor -.2 / near-1 ceiling and height-state gates are not integrated yet; recovery
of routine `20` does not imply recovery of all of `14060`.

Finite normal arithmetic is the established domain. An exactly coincident point
and reference has no defined normal in this native boundary and rejects the
transaction. SPU extended-exponent/denormal zero-distance parity is not claimed.
The query validates dimensions, team bit, finite vectors/radii and a usable
containment box. Invalid calls retain externally visible simulation state.
Original effect callback `1E880`, lifecycle, damage and capture remain suppressed.

## Validation

Input ELF SHA-256 remains
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`;
job hash remains
`9aca1695e6409668d9627ce865ad8008119f1c9e42d81a6106369f5a95e1872f`.
The private ps3recomp interpreter at revision
`a2c20108af45174dbb7d3e2f87f6acaaef09c6b4` executes unchanged original routines,
with the external effect callback intercepted at its return boundary. Seed
`207970` drives:

- 10,000 complete geometry queries with dimensions 1..24, edges, all four team
  bits and world-only selection;
- 10,000 point queries, with 4,460 contact cases;
- 5,000 full routine-20 motion responses, with 2,418 hit cases;
- **380,000 matching float lanes and 15,000 matching return values**.

Geometry returns all rectangle/corner lanes. Point and full-response cases compare
position, velocity and accumulated impulse plus contact return. Motion vectors
use XYZ with W zero. Original root/context inputs are supplied by the private
harness; this verifies the exposed routines, not complete mission caller state.
Original instructions/assets/harnesses remain outside tracked source.

The empty geometry stub failed its reference test. A live east-wall regression
failed with the previous adapter and passes after terrain integration: X stays
below the radius-adjusted wall, Z advances, and normal velocity reflects. Live
corner and coincident-point rollback regressions check both banks, cells, counts
and RNG. Public numeric references verify current versus cached position behavior.
Independent review found no actionable correctness issue.

Release builds with warnings as errors pass **32 native/CLI/graphics CTests** and
**233 Python tests**. Focused terrain/transport ASan/UBSan checks pass with host
leak inspection disabled. In original level 0, two units move and all 40 models
render in three frames with 11 textures and 10 complete framebuffer configurations
under llvmpipe. Unit 9 moves `(14.5379,3.4551)` to `(15.6851,4.56667)`.
Linux/Windows native CI validates the published PR.

## Remaining and estimate

Next recover original neighbor gathering/scheduling and the ground caller's
height/base/special-kind state, then hostile capture, weapons/damage, enemy AI
and mission win/loss. Automatic deployment, HUD/control parity, audio/save and
full-game parity remain unfinished.

Estimated capability: **55% toward a playable single-player mission**, up from
52%. This credits original terrain-wall/corner response and its live integration;
it is a capability judgment, not a percentage of original source recovered.
No combat or mission-completion credit is added.
