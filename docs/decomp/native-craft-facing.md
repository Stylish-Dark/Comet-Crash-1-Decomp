# Original ordinary craft movement-facing

Continues PR #57 (`b5e748b`), after both regression and native-source-port CI
workflows passed, including Linux graphics and Windows build/package jobs.

F3 craft facing now uses the original ordinary movement-command branch. It turns
at the source angular step, uses the source negative-Z convention, retains the
last facing when the command is neutral, and displays the prior quaternion for
one update. This replaces the native instantaneous atan2 of current velocity.

## Original evidence

ELF SHA-256 remains
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
FE5D8 job SHA-256 remains
`9aca1695e6409668d9627ce865ad8008119f1c9e42d81a6106369f5a95e1872f`.

| Range | Recovered behavior |
| --- | --- |
| `104E0`, optionally `11DB0..11DBC` | Target command uses source speed10/40 and optional .7 reduction |
| `10668..106AC` | Delta times bits `41490FDA` (~12.56637 radians/sec); controller byte `+3F==0` selects movement-facing |
| `11558..115A4` | Refined target-speed length; update only when strictly greater than bits `3C23D70A` (~.01) |
| `115A8..11614` | Refined reciprocal normalization; zero pitch/step/limits and source yaw step passed to `1A118` |
| `1A118` | Existing recovered aim math: angle wrap, bounded yaw change, source quaternion |
| `112D4`, `112E8`, `11300`, `1139C` | Save new entity quaternion, load old controller quaternion, display old, commit new to controller `+70` |

Controller `+5C` stores yaw. This is the ordinary movement-facing branch, not the
nearby target-facing branch, whose step includes an additional .9 multiplier.
The native adapter supplies the already-capped command, matching flight. Facing
responds to that command rather than coasting velocity. Output delay preserves
the original quaternion exchange even though yaw itself has already advanced.

`advance_arena_craft_facing` exposes yaw and pending quaternion separately and
returns the visible prior quaternion. Invalid commands, elapsed time or prior
state throw before mutation. Numeric work uses round-toward-zero and restores
caller rounding. The heading and position results are staged locally before
NativePilot commits either, keeping a failed frame atomic. Native pause/cancel
stops velocity and freezes facing; reset clears both yaw and pending orientation.

## Finite analytic boundary

The reused weapon-aim helper has explicit finite analytic limits for aligned
axes and zero-root cases. The IEEE instruction interpreter produces nonfinite
values for exactly +/-Z command alignment; it cannot establish original SPU
extended-exponent parity there. Native tests require finite unit quaternions for
these inputs. No exact-hardware parity is claimed for those limits, subnormals,
or the whole craft controller. This qualification also applies to the existing
weapon-aim recovery; it is not silently counted in the bit-exact cases below.

## Validation

A private oracle uses pinned ps3recomp interpreter
`a2c20108af45174dbb7d3e2f87f6acaaef09c6b4`. It executes untouched original target
scaling, `10668..106AC`, the selected `11558..11614` path, `1A118`, and the two
final quaternion stores. Entry registers/stack are staged explicitly, without
patching instructions or replacing helper calls. Allocation, graphics, effects
and unrelated state-machine branches are outside this oracle.

Seed **11558**, **10,000** cases: all **90,000 output float words match exactly**
(yaw, four pending quaternion lanes and four visible lanes). Cases vary capped
commands, delta, initial yaw/unit quaternion, speed modes and reduced-speed flag;
**1,000 neutral-command cases** preserve the complete prior state. Non-neutral
X magnitude is bounded away from zero before capping, avoiding the unsupported
aligned-Z IEEE limit. This is a stated comparison domain, not evidence that the
excluded hardware behavior has been recovered. Output SHA-256:
`953b1db273c3861cfd0a1718519d4986b21b6ffb75d42dbac82c44d37ee4427f`.
Twenty-four public numeric fixtures preserve the inputs and all nine outputs;
private original bytes and oracle remain outside Git.

Public tests pin delayed first turn, bounded turning, shortest-path angle wrap,
strict command threshold, neutral retention, maximum/zero delta, invalid-input
rollback, rounding restoration, aligned-axis finite limits, reversal, disabled
input freeze and reset. The pilot-facing assertion failed against the old native
velocity heading before implementation and passed with the recovered path.

Validation completed: **65 headless / 68 graphics-enabled CTests**, **233 Python
regressions**, **four focused ASan/UBSan CTests**, compileall, repository safety
and diff checks passed. Leak detection is disabled for this host. Independent
review found no actionable correctness issues and reran the two focused native
tests. Its optional aligned-axis test improvement was added and verified in the
final full suites. Graphics exercise synthetic geometry and virtual controller
input; physical-controller and fresh original-asset rendering are not claimed.

## Corrected recovery boundary and next work

Earlier handoffs called `3238` a craft world query. Inspection shows a
flag-`8000`-gated kind-7 request/queue construction path (`2DF8`, `2058`, `2198`),
not a demonstrated terrain-contact query. This work corrects that label. The
entity contact call at `1162C -> 7E40` is gated by entity byte `+51 & 1C` at
`104FC`; it must not be added unconditionally to ordinary flight. Its state
meaning and surrounding attached-peer update remain recovery work.

Native selects ordinary movement-facing (`+3F==0`), mode1 and reduced-speed0.
Initial pilot height, input bindings/lifecycle and preview control/render
ownership remain native policy. Target-facing, banking/action/attachment state,
body registration/player links, carrying/economy/damage, terrain/peer-contact
scheduling, aircraft, original mission/campaign/AI and HUD/audio/save remain open.

Estimate stays **92% toward the first playable single-player mission**. This
recovers visible craft-control behavior but adds no completed carrying/combat
interaction. The estimate is not full-port completion or source-code coverage.
