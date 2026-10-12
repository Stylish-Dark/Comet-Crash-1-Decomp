# Native construction integration and level scenarios

This continuation starts from GitHub commit `7f90643`. It reconstructs the
previous local construction integration after its working checkout was lost;
it does not claim to reproduce the missing commit hashes. The new bounded
milestone is an executable, assertion-based scenario runner over actual maps.

## What works

- Native-owned player resources, construction ring, routing grid, shared RNG,
  event heaps, both entity banks, cell lists and free-index pool.
- All eight recovered structure constructors: opcodes 20–26 and 28.
- Eligibility checks, completion debits, allocation, failed-commit reservation
  release and deterministic timed construction dispatch.
- Strictly due events (`event.time < clock`), monotonic finite clocks, one
  completion per tick, player order, retries and bounded queue backpressure.
- Unsupported event selectors stay queued and are reported; they are not
  silently consumed or turned into invented actions.
- Eligible existing map structures are constructed and settled into both banks
  without spending player resources. Source orientation and type-28 variant
  are retained. Gates use explicit configured teams. Shared-cell visibility is
  resolved using the final map cell's referenced entity.
- The scene projects successful live constructions into the existing renderer.
  Rocks, bases and structures requiring unrecovered upgrade transitions remain
  display-only and keep their map visibility.
- Keyboard construction and paused/automatic event clock advancement in the
  native SDL/OpenGL viewer.

Newly constructed structures now advance through the recovered original core
progress update and become ready. Units, weapons, combat, income and victory
remain absent. Readiness means the original core flags/counters are settled;
it does not imply weapon behavior. Explicit ground-unit spawning is now
available; see [native-live-ground-spawn.md](native-live-ground-spawn.md). Factories now advance
original production timers and reserve pending units when explicitly enabled. Map settlement
still applies only to pre-existing structures at or below the initial level.

## Build and run

```sh
cmake -S decomp -B build/native -DCMAKE_BUILD_TYPE=Release
cmake --build build/native
ctest --test-dir build/native --output-on-failure
build/native/comet_construction_replay --map /path/to/extracted/data --level 0 < decomp/examples/construction-level0.scenario
```

`--map` uses the existing native level loader. Without `--map`, the scenario
starts from an empty grid. Proprietary maps and assets must be provided locally;
none are included in the source checkpoint.

The new runner returns nonzero for malformed commands or failed expectations,
with the input line in its error. This allows a level scenario to serve as a
repeatable integration gate rather than relying on visual inspection.

The bundled level-0 scenario explicitly uses synthetic tuning and resources.
It checks eight imported live structures, a manual opcode-20 build, and a
scheduled opcode-21 build: ten allocated entities, 147 displayed entities,
zero pending ring entries, resource balance 370, and the one original map event
still in the heap. Those counts describe the supplied original level-0 map;
they are intentionally checked rather than assumed for other maps.

## Configuration and commands

Blank lines and `#` comments are permitted. Lines are limited to 4096 bytes;
integer ranges, finite floats, trailing tokens and duplicate tuning are checked.

```text
world capacity seed
player base_x base_z team resource resource_max inactive
# One player line for each of 1..4 map players.
tuning opcode scalar64 denominator first_byte second_byte scalar84
production 0_or_1
unit-quota owner active capacity
unit-base owner 0_or_1
unit-target owner target_or_255
run
```

Capacity is at most 65536; teams are 0..3; denominator must be positive.
Tuning is supplied explicitly because the title's mutable initialization tables
are not recovered here. Do not describe the example as authentic game balance.

Production defaults to disabled and quota capacity to zero. Quotas are explicit
test inputs; authentic capacity initialization remains unrecovered.

After `run`, the replay runner accepts:

| Command | Effect |
| --- | --- |
| `queue owner opcode x z` | Submit a manual build and print its eligibility status |
| `step` | Process one construction-ring request |
| `drain` | Process the ring until empty; does not advance event time |
| `event owner time selector opcode x z` | Schedule an explicit event |
| `advance absolute_time` | Age existing builds, dispatch events and process one ring request |
| `expect ready N` | Count supported live structures with the original ready flag |
| `expect allocated N` | Check occupied live entity slots |
| `expect pending N` | Check pending ring entries |
| `expect display N` | Check rendered entity count when using a map |
| `expect resource0 N` | Check player 0's whole-unit balance; suffix selects player |
| `factory entity_index selector` | Select factory mode with selector 4..8 |
| `expect-mode entity_index mode` | Check production mode |
| `expect unit-pending0 N` | Check player 0 pending-unit total; suffix selects player |
| `expect-factory-ground-unit index target status` | Check completed ground template binding: no-target, blocked or bound |
| `expect-factory-unit index kind model` | Check original selected-unit template kind/model without mutation |
| `expect-unit owner kind N` | Check pending count for unit kind 40..48 |
| `expect events0 N` | Check player 0's heap size; suffix selects player |

Viewer configuration uses the same header through `run`, with no scenario
commands afterward. Build with `-DCOMET_NATIVE_VIEWER=ON` and SDL2/OpenGL:

```sh
build/native/comet_native /path/to/extracted/data 0 --assets /path/to/extracted/data --arena --construction-config decomp/examples/construction-level0.cfg
```

Arrows move the highlighted cell; 1..8 choose opcodes 20–26/28; Tab changes
owner; B or Space submits a build and advances one tick; T advances a tick;
P toggles continuous clock advancement. The viewer starts paused. The title
shows owner, cell, opcode, resource balance, clock mode and the selected live
structure's percentage or ready status. Cyan lines show progress at build cells.

## Validation recorded for this continuation

- Release CMake build with warnings treated as errors and all CPU/CLI tests.
- Existing Python regression suite: 233 tests.
- SDL/OpenGL offscreen rendering, DDS sampling, missing-model rejection and
  injected keyboard construction with verified debit and scene projection.
- Actual original level 0: 40 model entries, 28,890 vertices and 10,202 triangles;
  two-frame construction scene and the assertion-based scenario above.
- Private original-instruction checks: 1,000 initializer fixtures, 3,500
  structure-constructor fixtures and 1,000 map-settlement fixtures. All compared
  complete output bytes, with deterministic seeds 1187, 2634 and 7331.
- Address/undefined-behavior sanitizer run over construction integration tests
  (leak detection disabled for this execution environment).

Private game bytes, decoded-original fixtures and build outputs remain outside
tracked source. Windows graphics/build behavior has not been validated here.

## Construction readiness milestone

Current validation: all 19 CPU/CLI CTest checks, 233 Python regressions,
SDL/OpenGL offscreen rendering with injected build/tick/ready controls,
and construction/projection tests under AddressSanitizer and UBSan pass.
The original level-0 replay reaches 10 ready structures at clock 4; the map's
existing event also leaves an eleventh newly allocated structure in progress.

The FE5D8 SPU job's construction arm at `0x4DDC..0x4F18` updates health,
progress and quantity, then invokes settlement `0x1FEE8..0x1FFB4`. Its duration
table at `0x20BE0` contains 2 for all ten type rows and five levels. The native
leaf supports the eight recovered buildable constructors, levels 1–5; other
state is retained without invention. The first-update bit is cleared without
allocating the original transient effect. Selling/other timer arms, effects,
sound and completion notifications remain outside this boundary.

The leaf preserves SPU round-toward-zero arithmetic, denormal flushing and the
original reciprocal estimate/refinement sequence. The caller's rounding mode
is restored. Floating overflow/nonfinite core inputs are rejected as unsupported.
This finite gameplay boundary does not emulate SPU extended-range floats.
Existing builds age by the elapsed clock interval before new requests are
constructed, and both native banks receive the same result. A new entity starts
aging on the next tick. The ready transition is reported once per entity.

Private validation executes the untouched original instructions through the
core boundaries with synchronous DMA and empty auxiliary effect pools. Across
2,500 complete 256-byte fixtures (seed 9417), all bytes match the native leaf:
eight types, five levels, first-update and subsequent states, varying progress,
health, quantities, counter wrap, zero/normal/threshold/overshooting intervals.
The interpreter is ps3recomp commit `5244db92669b54831c6d3ebb7a96d58c4dbfa6df`;
original bytes and private harnesses are untracked. This comparison validates
core state, not omitted audiovisual side effects. Public tests retain exact
arithmetic/threshold vectors, rounding restoration and unsupported-state checks.

## Remaining playable-port milestones and estimate

**Rough estimate: 41% toward a playable single-player mission**, using the
weighted capability checklist below. This is a planning estimate, not a
measured fraction of all original code or a full-release completion claim.
The prior capability checkpoint scores 25 under this same rubric; verified
construction progression/readiness contributes 5, and verified factory timing
and quota reservations contribute 3; unit initializer/claim primitives add 2.
Ground constructor binding/heading adds 2; verified statistics registration and
its quota bridge add 1. Live ground allocation, failure accounting, owned-world
integration and visible deployment add 3. Live units remain stationary.

| Capability | Weight | Earned | Remaining |
| --- | ---: | ---: | --- |
| Asset decoding, map import and native scene rendering | 15 | 15 | Improve original visual fidelity |
| Construction and economy | 20 | 15 | Authentic tuning/income, base builds and upgrade transitions |
| Unit production, movement and targeting | 20 | 11 | Original producer placement, aircraft binding, movement and targeting updates |
| Weapons, damage and enemy behavior | 20 | 0 | Combat and AI |
| Mission rules and win/loss/restart | 15 | 0 | Complete a mission loop |
| Gameplay UI, audio and distribution | 10 | 0 | Normal controls, feedback, save and release validation |
| **Total** | **100** | **41** | |

Factory production selection is now implemented; see
[native-factory-controls.md](native-factory-controls.md). Next: recover the
original live ground-unit update path in this same simulation, including
route following, cell migration and target updates. Producer placement remains
unrecovered. Remaining
event selectors and higher map upgrades also need recovery. Windows builds/graphics remain unvalidated. Synthetic example tuning
is explicit test configuration, not recovered title balance.

Factory timing/reservation validation and the level-0 production scenario are
recorded in [native-factory-production.md](native-factory-production.md).

Unit initializer prefixes, quota claims and their remaining spawn boundaries are
recorded in [native-unit-templates.md](native-unit-templates.md).

Completed ground constructor binding/heading and remaining allocation boundaries
are recorded in [native-ground-unit-binding.md](native-ground-unit-binding.md).

Verified entity registration, capacity side effects and the quota bridge are
recorded in [native-unit-registration.md](native-unit-registration.md).

Live allocation, explicit spawn controls and remaining movement/producer
boundaries are recorded in [native-live-ground-spawn.md](native-live-ground-spawn.md).
