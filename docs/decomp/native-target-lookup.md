# Original target lookup and executable factory events

This milestone corrects the CA2E0 stage predicate in both native target resolvers
and makes scheduled production-mode commands executable in the owned simulation.

## Correct rule

The original function checks visibility ownership and its exclusion mask first.
It then scans packed cell entries in order. `entry[7] & 0x1C` must equal `0x0C`
or `0x10` (stages 3 or 4), and that entry's owner must not be excluded. The first
matching record wins. Missing targets return -1.

At `CA394..CA3E0`, the branch to CA3E0 skips a record when neither stage matches.
The old native condition did the reverse. The same rule appears in the second
entry and unrolled loop. Full-function validation covers every path rather than
inferring the rule from one block.

This correction applies to the packed live-storage resolver and the lightweight
ArenaCellEntry resolver. It does not modify entity stages or construction flags.
The constructors' stage-4 factory records are already valid targets, including
before construction readiness. The production-selection command itself has no
ready-state gate. Other selectors remain outside the recovered consumer scope.

## Resulting behavior

- A due selector-4..8 event targeting a live type-26/28 factory now selects its
  production mode, updates both banks and consumes the pending command.
- Equality with the event time remains idle; dispatch requires time < clock.
- Missing/masked/mismatched targets still retry through the original RNG path.
- Mode selection leaves funds and allocated-entity count unchanged.
- The synthetic factory replay completes a scheduled mode change at clock 4.
- `factory-level0.scenario` builds a type-26 factory at (14,2) on original level
  0 and executes selector 8 at clock 2: mode 4, nine ready/allocated structures,
  player-0 balance 340, and the map's later event remains queued.

## Verification

A private Capstone evaluator executes the untouched CA2E0 instructions from
`CA2E0..CA510`, including original branches and counter-driven unrolled traversal.
Across 5,000 fixtures (seed 126204), the native resolver returns exactly the same
index: arbitrary visibility bytes, 32-bit masks, owners/stages/indices and list
lengths 0..126. Proprietary bytes and evaluators remain untracked.

Public tests cover matching stages, ownership exclusions, no visibility,
ordered live-bank lookup, strict event timing, successful factory dispatch,
missing-target retry and unchanged funds/capacity. Full CPU/CLI regressions,
Python tests and affected sanitizer checks are run for this milestone.

Release validation passed all 20 CPU/CLI checks, 233 Python tests, both factory
scenarios and the level-0 construction scenario, plus the affected state/storage/
simulation tests under ASan and UBSan.

Factory timing and pending-unit reservations are now implemented; see
[native-factory-production.md](native-factory-production.md). The unit initializer
prefix and quota claim are covered in [native-unit-templates.md](native-unit-templates.md).
Live-unit allocation, binding, movement and combat remain unimplemented.
