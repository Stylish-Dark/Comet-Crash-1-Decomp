# Original unit templates and quota claims

This milestone builds on `df118d8`. The native port now initializes the recovered
core/template state for all nine produced unit kinds, and provides the original
quota-claim operation for the future spawn consumer. Factory inspection uses the
initializer without changing the simulation. These are verified prerequisites;
this checkpoint has no live-unit spawn, movement or combat loop. Subsequent
[live ground spawning](native-live-ground-spawn.md) connects the ground
constructors to explicit allocation and display; movement/combat remain absent.

## Original initializer boundary

FE5D8 `14B30` initializes common extra state, consumes one shuffled RNG value,
dispatches kinds 40..48, calls the shared core initializer at `2DF8..3060`, and
finishes per-kind fields. The native boundary ends at `14FD8`, immediately before
`12BE0` performs world-dependent target/cell binding. Subsequent grid lookup,
target assignment and initial velocity/orientation selection are excluded.
The initializer preserves untouched bytes when reusing an entity slot.

| Kind | Model slot | Asset | Packed stage | Core quantity | Core `+64` |
| ---: | ---: | --- | ---: | ---: | ---: |
| 40 | 2 | scout02 | 5 | 10 | 50 |
| 41 | 3 | fighter02 | 5 | 10 | 170 |
| 42 | 4 | flyer | 6 | 15 | 100 |
| 43 | 5 | beetle | 5 | 10 | 320 |
| 44 | 6 | stunner02 | 5 | 10 | 600 |
| 45 | 7 | converter02 | 5 | 10 | 1200 |
| 46 | 8 | agentAdv_Medic | 5 | 10 | 600 |
| 47 | 9 | agentAdv_Carrier | 5 | 10 | 350 |
| 48 | 10 | agentAdv_Laser | 6 | 45 | 2000 |

Asset labels are from the existing recovered model manifest. Offset-based
scalars retain their original float bits; their precise movement/weapon meaning
is not assigned without recovering their consumers. Unit state uses packed
stages 5/6, distinct from the structure target-lookup stages 3/4.

The port retains the original round-toward-zero conversion and ordered arithmetic,
including the reciprocal refinement and angle calculation using one RNG draw.
The caller's rounding mode is restored. Kind/owner validation and finite
positions in the signed-halfword coordinate domain occur before mutation or
RNG consumption. Original cell/target binding requires additional world state.

## Pending-to-active quota claim

The pure helper replaces the state transition in `BA08..BB28`, called at `DABC`.
It tests the requested kind's pending halfword at `+96 + 2*(kind-40)` decimal.
If zero, it returns no kind and changes nothing. Otherwise it decrements that
halfword and the total pending halfword at `+94`, increments the active halfword
at `+90`, and returns the actual kind 40..48. Halfword wrap matches the original.
Capacity is not checked again: this operation claims an already-reserved unit.
The helper is intentionally separate from live allocation, and is not called by
the owned simulation until the spawn path is implemented. Its raw halfword
semantics are broader than the simulation's validated quota domain.

The original wraps this transition in atomic DMA with retry on failed commit.
Native owned state needs no DMA; private verification uses successful synchronous
atomic DMA. This proves the state/return boundary, not thread contention behavior.

Tracing also establishes that quota claim and allocation helpers are in the
same FE5D8 job: `DABC` calls `BA08`, `DB80` calls `C380`, and the latter uses
`14B30`, player-statistics registration `48C8`/`BB30` and allocator `2058`. The prior description
of allocation as belonging to another original job was too specific and has
been corrected. The complete spawn caller and its failure handling still need
recovery before these leaves can be assembled faithfully.

## Native inspection and reproduction

`ArenaConstructionSimulation::factory_unit_template(index)` derives the unit kind
from a supported factory's current mode and initializes a fresh inspection slot
at the factory position. It uses a copy of the simulation RNG, preserving future
random outcomes, funds, quotas and live entity banks. Inspection is available
before readiness, matching the original factory preview's lack of a ready gate.
It does not emulate the factory preview animation or spawn-position jitter.

Replay adds `expect-factory-unit index kind model_slot`, with strict bounds and
nonzero exit on a missing/mismatched factory. Examples:

```sh
build/native/comet_construction_replay < decomp/examples/unit-templates.scenario
build/native/comet_construction_replay --map /path/to/extracted/data --level 0 < decomp/examples/production-level0.scenario
```

The synthetic scenario exercises both factory types and all ten mode choices
covering nine distinct units. It preserves two allocated structures and balance
190 after inspection, then verifies one pending kind-40 order. The original
level-0 scenario additionally verifies templates for the imported factory,
the manually built mode-4 factory, and the original event's factory, retaining
four pending orders, ten allocated structures and balance 280.

## Validation

- Red/green tests for initializer support, quota claim, simulation inspection
  and replay expectations. Committed tests cover exact arithmetic vectors,
  all model selections, reused-byte preservation, unsupported input, RNG
  consumption, rounding restoration, ordinary claims and signed-halfword wrap.
- Private untouched-instruction comparisons: 5,000 complete 256-byte initializer
  slots (seed 140530) and 5,000 complete 128-byte quota blocks plus return values
  (seed 140508). Randomized reused bytes, all kinds, owners 0..3, seeds, coordinate
  boundaries, empty claims and counter wrap are included. An independent review
  also checked 864 initializer boundary fixtures and found no material defect.
- Fresh Release build with warnings treated as errors and all 22 CPU/CLI tests.
- All 233 Python regression tests.
- Ten affected state/storage/construction/unit/production/replay tests under
  AddressSanitizer and UBSan.
- Offscreen SDL/OpenGL test with injected build/tick/mode controls and verified
  unit-template inspection on the constructed factory.
- Both assertion-based scenarios above.

Original executable bytes, disassembly, private fixtures, interpreters and build
outputs remain untracked. Windows behavior has not been validated.

## Remaining and estimate

Ground constructor binding/heading is now complete for seven kinds; see
[native-ground-unit-binding.md](native-ground-unit-binding.md). Next: recover the
statistics-registration and allocation/failure path, then connect constructors
and quota claims to live entities. Movement, targeting updates, damage/weapons,
enemy behavior and mission rules remain absent. This template milestone advanced
the planning estimate to 35% toward a playable single-player mission, with two
additional points for initialization/claim primitives.
This is not a claim that units now spawn or that the unit loop is complete.

Live ground allocation and explicit deployment controls are now recorded in
[native-live-ground-spawn.md](native-live-ground-spawn.md).
