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

No timer-driven construction readiness, units, weapons, combat, income or
victory loop is implemented by this integration. A constructed entity still
has the original constructor's in-progress state. Map settlement only applies
to pre-existing map structures at or below each constructor's initial level.

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
run
```

Capacity is at most 65536; teams are 0..3; denominator must be positive.
Tuning is supplied explicitly because the title's mutable initialization tables
are not recovered here. Do not describe the example as authentic game balance.

After `run`, the replay runner accepts:

| Command | Effect |
| --- | --- |
| `queue owner opcode x z` | Submit a manual build and print its eligibility status |
| `step` | Process one construction-ring request |
| `drain` | Process the ring until empty; does not advance event time |
| `event owner time selector opcode x z` | Schedule an explicit event |
| `advance absolute_time` | Dispatch eligible events and process one ring request |
| `expect allocated N` | Check occupied live entity slots |
| `expect pending N` | Check pending ring entries |
| `expect display N` | Check rendered entity count when using a map |
| `expect resource0 N` | Check player 0's whole-unit balance; suffix selects player |
| `expect events0 N` | Check player 0's heap size; suffix selects player |

Viewer configuration uses the same header through `run`, with no scenario
commands afterward. Build with `-DCOMET_NATIVE_VIEWER=ON` and SDL2/OpenGL:

```sh
build/native/comet_native /path/to/extracted/data 0 --assets /path/to/extracted/data --arena --construction-config decomp/examples/construction-level0.cfg
```

Arrows move the highlighted cell; 1..8 choose opcodes 20–26/28; Tab changes
owner; B or Space submits a build and advances one tick; T advances a tick;
P toggles continuous clock advancement. The viewer starts paused. The title
shows owner, cell, opcode, resource balance and clock mode.

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

## Remaining playable-port milestones

1. Recover and execute the original construction/upgrade progress update so
   newly built structures become operational through verified timing.
2. Recover unit production, movement and targeting, then weapons and damage.
3. Recover authentic economy and mutable tuning/bootstrap inputs.
4. Execute remaining event selectors, base construction and higher map upgrades.
5. Integrate mission outcome, restart/save behavior and normal gameplay UI.

There is no defensible overall completion percentage yet. Progress is reported
as validated capabilities and remaining playable-loop milestones rather than
repeating a speculative 20–25% figure.
