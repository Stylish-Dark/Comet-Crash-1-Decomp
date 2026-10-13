# Original ground route steering

The complete FE5D8 simulation-job leaf `13998..1405C`, called by `14060`, now
has a native implementation in `arena_ground_steering.cpp`. Live opt-in ground
transport supplies target bases and team visibility bits to this implementation.
This replaces the preview's immediate route-vector choice; the full unit physics
and the original caller's context preparation are still incomplete.

## Recovered behavior

The query preserves the original distinction between target availability,
direct routing, initial blocked state and an auxiliary suppressed cell. Position
lanes are clamped to `[0, extent - 0.5]` before cell selection. At the selected
base cell, steering points toward its center and clears blocked status. Without
a target, quaternion-rotated negative Z provides the forward fallback. Direct
routing decodes the current route even when its blocked bit is set. Otherwise a
blocked current route falls back to forward and sets blocked status.

Normal routing starts with the directed pair's fixed route vector, then examines
west, east, negative Z and positive Z neighbors in that order. Sample coordinates
are clamped; candidate offsets retain the original cardinal step. The auxiliary
cell can suppress a sample. The owning team's visibility bit substitutes the
primary vector for a neighbor's vector. A candidate needs its direction dotted
with the cardinal step above float bits `B8D1B717`, its direction dotted with the
primary below 0.5, and a strictly shorter candidate distance. Neighbor blocked
bits are ignored by this leaf. The selected output is the cardinal step.

Quaternion operation order, inverse-root estimate buckets/refinement, fused
operations and round-toward-zero arithmetic follow the original instruction
sequence. Native finite-input/extent checks and a zero-distance guard bound the
API. Extended/denormal SPU hardware equivalence is not claimed.

## Integration and evidence

Transport uses recovered X/Z steering scaled by the existing unit speed. Base
centering therefore slows and settles the unit instead of carrying it past the
base. The existing conservative route-obstruction checks, synchronous bank/cell
migration and fixed altitude remain native policies. The normal simulation
supplies configured team bits; bases-only API calls default to the player's team.
The auxiliary suppressed cell defaults to absent. Target selection/bootstrap
and these configured caller inputs are not established as original mission state.

A private interpreter executed the untouched original leaf from the revalidated
ELF SHA-256 `3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
5,000 randomized complete queries matched all four direction float bit patterns
and blocked status. Queries vary position, orientation, extent, route/visibility
grids, owner/target, flags and suppressed cells. The private interpreter is from
ps3recomp revision `a2c20108af45174dbb7d3e2f87f6acaaef09c6b4`; neither it nor
original instructions/assets are shipped. Eight original numeric reference
outputs are committed as portable regressions alongside branch behavior tests.

`decomp/examples/ground-steering.scenario` builds and explicitly deploys a unit,
checks movement and pause, resumes and verifies arrival within 0.01 of (10.5,
10.5), both-bank cell membership and unchanged resource/allocation counts. The
observed position is (10.4995, 10.4995). Replay's `expect-unit-near` command also
rejects malformed/nonfinite inputs. This is synthetic tuning, not a mission.

Validation: 28 native/CLI/graphics CTests and 233 Python regressions pass; focused
steering/transport tests pass ASan/UBSan. Independent review caught a team-versus-
owner visibility default; its regression failed before the correction and passes
after it.

## Remaining

Recover the surrounding `14060`/`158B8` dynamics: acceleration, damping, collision,
crowds, height and special-kind auxiliary behavior, plus target transitions.
Automatic producer placement, aircraft, weapons/damage/enemy behavior, mission
win/loss, HUD/input feedback, income/balance and audio/save remain incomplete.
The capability estimate is 46% toward a playable single-player mission, up from
43%; three points credit verified original steering and live base arrival.


Current continuation: [original velocity dynamics](native-ground-dynamics.md)
adds selectable acceleration, drag and horizontal speed limiting. The 46%
checkpoint above remains the steering milestone's estimate; current is 48%.
