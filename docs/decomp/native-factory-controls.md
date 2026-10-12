# Native factory production controls

This milestone builds on `f2e5896` and implements the original production-mode
command for structures 26 and 28. It does not spawn units or advance a guessed
production timer.

## Recovered boundary

The selector jump table at `CCA60` sends selectors 4..8 to `CDA88`.
`CDA88..CDA98` accepts types 26 and 28 and branches to `CCC04..CCC24`:

| Command selector | Stored production mode |
| ---: | ---: |
| 4 | 0 |
| 5 | 1 |
| 6 | 2 |
| 7 | 3 |
| 8 | 4 |

The command stores selector minus four at entity byte `+AA`. If that mode differs
from the old byte, it first clears the word at `+80`. Choosing the same mode
preserves all bytes, including an accumulated production timer. The original
command has no construction-readiness gate, debit, allocation or level-unlock
check. Unsupported kinds/selectors leave the native slot unchanged. The native
simulation updates both banks from the selected bank's slot.

The existing pending-command consumer now accepts these already-resolved
factory commands and returns the player to idle mode 11 after application.
Supported timed events enter the original F1030/CA2E0 lookup and retry path;
unknown command selectors remain queued as before.

## Native controls

The viewer uses F5..F9 to select modes 0..4 on the selected cell's live factory.
The current mode is shown in the title. Manual cell selection requires that
factory to belong to the selected player; this is a native viewer policy, not
an added gate on the recovered command. Direct indexed replay/pending commands
operate on their resolved live entity index.

Replay commands:

| Command | Result |
| --- | --- |
| `factory entity_index selector` | Apply selector 4..8; report changed/unchanged, fail on invalid target |
| `expect-mode entity_index mode` | Assert the actual factory mode |

Run the synthetic example:

```sh
build/native/comet_construction_replay < decomp/examples/factory-controls.scenario
```

Scene projection records a production mode only for live type-26/28 entities,
clears stale derived fields during bootstrap, and rejects out-of-range map cell
links. Explicit index selection avoids ambiguity on cells shared by records.

## Validation

- Red/green regression: before implementation, resolved factory commands were
  rejected as unsupported; the new command path applies them and consumes the
  pending command without spending resources or allocating slots.
- Private Capstone evaluator executes unchanged original instructions in
  CDA88..CDA98 and CCC04..CCC24. All 256 output bytes match in 5,000 randomized
  fixtures, seed 12048, including both factory types, other entity kinds,
  all five selectors, changed/unchanged choices and arbitrary timer words.
- Public checks cover complete-slot preservation, both banks, pending command
  consumption, original target retries, scene projection and malformed links,
  and replay success/failure cases.
- Full Release CPU/CLI suite, Python regression suite, offscreen SDL build/tick/
  mode controls, and factory/construction tests under ASan and UBSan.

Original binaries and private evaluators stay untracked.

## Remaining production blockers

CA2E0 accepts stage-3/4 packed records, including the factory constructors.
The native resolver previously inverted that test. The corrected resolver now
lets due selector-4..8 events select factories, with original ownership masks,
ordered lookup and missing-target retries. No invented stage transition is
needed. See [native-target-lookup.md](native-target-lookup.md) for validation.

The factory timer and pending-unit capacity reservations are now implemented;
see [native-factory-production.md](native-factory-production.md). The next
bounded recovery target is completing the original binding/placement and
allocation/failure path. Unit initializer and claim boundaries are covered in
[native-unit-templates.md](native-unit-templates.md). Production-mode selection by itself received no
additional completion credit.
