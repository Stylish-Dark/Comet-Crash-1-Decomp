# Unit registration implementation plan

> **For agentic workers:** Use superpowers:executing-plans for inline execution.

**Goal:** Recover player-statistics registration needed before live allocation.

**Architecture:** A raw 128-byte big-endian statistics block keeps unknown fields intact. A typed offset-based delta ports BB30; entity registration ports 48C8; quota codecs bridge the existing production claim without inventing live spawning.

**Tech Stack:** C++20, CMake/CTest, private original SPU interpreter.

**Spec:** Original FE5D8 BB30..BF54 and 48C8..4B64; documented in docs/decomp/native-unit-registration.md after validation.

## Global constraints

- Modify only this repository; no merge.
- Original instructions and interpreter remain ignored under build/recovery.
- Preserve wrap, signed capacity comparisons, and writes on capacity rejection.
- Use offset names where field meanings remain unknown.
- Reject nonfinite or unsupported float range atomically; restore host rounding.

## Review focus

- Capacity rejection still changes unrelated fields.
- Active count is already incremented by production claim.
- Signed halfword and byte wrap are intentional.
- Signed capacity words include negative values.
- Unsupported numeric inputs must preserve all bytes.

### Task 1: Delta and registration recovery

Files: create decomp/include/comet/arena_player_statistics.hpp, decomp/src/arena_player_statistics.cpp, decomp/tests/arena_player_statistics.cpp; modify decomp/CMakeLists.txt.

Interfaces: apply_arena_player_statistics_delta(stats, delta) returns capacity acceptance; register_arena_entity_statistics(stats, slot, adding, count_active, auxiliary) distinguishes registered, capacity rejected and unsupported entity; read/store quota codecs share the original offsets with ArenaUnitProductionQuota.

- [x] Add failing tests for rejection side effects, equal capacity, wrap, stage routing, claim/registration/rollback and numeric guards.
- [x] Implement recovered arithmetic and raw-field updates.
- [x] Compare complete 128-byte results and return codes with untouched instructions.
- [x] Add original numeric regression vectors and run fresh Release, Python and sanitizers.
- [x] Review, document actual boundary and capability score, commit and publish an authorized branch.
