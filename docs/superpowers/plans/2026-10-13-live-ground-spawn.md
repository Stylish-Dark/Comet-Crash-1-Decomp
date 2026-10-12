# Live ground spawn implementation plan

> **For agentic workers:** Use superpowers:executing-plans for inline execution.

**Goal:** Turn pending ground orders into allocated, visible units.

**Architecture:** Port C380 jitter and compose the existing constructor, binding,
claim and registration leaves. An explicit native allocation adapter commits to
both owned banks and actual-cell records. Expose spawn at supplied coordinates
through simulation, scene, replay and viewer; original automatic placement is
outside this milestone.

**Tech stack:** C++20, CMake/CTest, SDL2/OpenGL, private original interpreter.

**Spec:** Original FE5D8 C380, 2058 and 1F8B0, with existing ground constructors
and statistics registration. Native policies are documented separately.

## Global constraints

- Keep original bytes/interpreter ignored under build/recovery.
- Seven ground kinds only: 40, 41, 43..47; no aircraft spawning.
- No guessed movement, producer placement or combat.
- Preserve statistics and RNG effects on known capacity/allocation failures.
- Invalid native requests preserve state; reject before consuming an order.
- Do not merge; publish only the authorized working repository branch.

## Review focus

- Claim already increments active; registration must not increment it again.
- Pool exhaustion consumes the order and compensates active/weighted state.
- Unit model marker differs from the structure marker.
- Units must not occupy structure lookup links or progress as buildings.
- Jitter crossing cell edges must keep storage and scene consistent.

### Task 1: Spawn core and allocation adapter

Create arena_unit_spawn.hpp/.cpp and arena_unit_spawn.cpp tests. Implement
initialize_arena_ground_unit_spawn and spawn_reserved_arena_ground_unit.

- [x] Write failing tests for success, empty/unsupported requests, full pool,
  both banks/records, target binding, rounding/RNG and cell crossing.
- [x] Implement and compare original jitter/constructor/registration and
  allocator records, preserving the stated native adapter boundary.

### Task 2: Simulation and scene integration

Modify arena_construction_simulation.hpp/.cpp and native_construction_scene.hpp/.cpp.
Add spawn_reserved_ground_unit(owner, kind, supplied position), owned raw player
statistics synchronized with quota reservation, and scene projection.

- [x] Write failing integration tests for quotas, visible units, unchanged
  structure-cell links and later ticks preserving unit bytes.
- [x] Implement explicit spawning; keep automatic producer placement absent.

### Task 3: Reproduction, controls and completion

Modify construction_replay.cpp, replay tests, native_viewer.cpp and viewer tests.
Add an explicit spawn command, live-unit expectations and U at selected cell.

- [x] Add failing replay/control tests, implement, then validate source level 0.
- [x] Run full Release/Python, affected sanitizers and offscreen rendering.
- [x] Obtain independent review, document progress and remaining boundaries,
  commit and publish the validated branch.
