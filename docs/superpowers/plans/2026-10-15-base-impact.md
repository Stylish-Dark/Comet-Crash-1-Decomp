# Enemy-base impact implementation plan

> **For agentic workers:** Use superpowers:executing-plans for inline implementation.

**Goal:** Moving ground units can hit enemy bases, update recovered accounting, leave play, and release quota and entity capacity.

**Architecture:** Recover the 6F40 arrival selector and BF58 player-statistics leaf independently. Integrate after each native movement substep. Native owned storage removes both cell entries and recycles indices FIFO; this deliberately omits the original asynchronous tombstone delay, messages and effects.

**Tech Stack:** C++20, CMake/CTest, Python replay tests; private original-instruction comparison.

**Spec:** This document records the bounded design inferred from original job FE5D8: 6F40..7130, 7290..735C, 6578..6724, BF58..C170, 29F8..2AD0.

## Global constraints

- No original game bytes, disassembly or generated instruction implementations in Git.
- Base counters are explicit native bootstrap inputs; no guessed mission defaults or win/loss rules.
- Existing scenarios remain unchanged unless base impacts are configured.
- Preserve synchronous bank consistency, ordered cells and native rollback boundaries.

## Review focus

- Strict radius and vertical distance; last eligible base wins.
- Captured ownership affects attribution and quota release.
- Duplicate removal, invalid owners and malformed membership do not corrupt storage.
- Recycled indices never leave stale display projections or structure links.
- Zero counters stay zero; disabled counter changes preserve attribution.

### Task 1: Recover leaves and owned removal

Files: new `arena_ground_base.hpp/.cpp`, existing player statistics and storage headers/sources, new `tests/arena_ground_base.cpp`, CMake.

- [x] Write tests for radius/team/availability, base-counter clamp and attribution, resource cap, FIFO removal and duplicate rejection.
- [x] Build test and observe missing-feature failure.
- [x] Implement typed arrival selection, BF58 resource/counter update and owned removal.
- [x] Run leaf tests and original-instruction comparisons.

### Task 2: Connect the playable native path

Files: construction simulation/config, scene projection, replay CLI/tests, example and recovery documentation.

- [x] Write failing scenario: spawn, travel, impact, counter decrement, quota release, recycle and repeat.
- [x] Add optional `base-counter owner value`, tick impact records and counter inspection.
- [x] Remove retired projections before projecting newly constructed objects; remap structure links.
- [x] Run all native/graphics/Python tests and sanitizers, independent review, publish stacked PR.

## Execution record

Baseline: 34 CTests passed after restoring an executable bit lost in the host
snapshot. New leaf target failed for absent header/API; integration target
failed for absent counter/tick API; replay failed on unrecognized base-counter
configuration. Each then passed after implementation. Final validation and
boundaries are recorded in `docs/decomp/native-ground-base-impact.md`.
Independent review: no blocker. Minor follow-ups: combined capture-to-impact
integration and explicit simulation failure-rollback coverage.
