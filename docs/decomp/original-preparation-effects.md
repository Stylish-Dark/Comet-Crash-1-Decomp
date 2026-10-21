# Original player preparation: consumed effects and counted requests

This continues PR #61 atfd7339661dd209ea6a7f4b60eacc9d6fe9557e97. It recovers
9EC8..A268 after the existing9A70 staging/1AD8 consumer, plus3510..387C counted
request construction. Together these implement the original type1 preparation
routine within the explicit supported numeric domain and publication boundary.
Full10290 controller/actions/carry remains the next major gameplay dependency.

Reference EBOOT SHA256:
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
FE5D8 job SHA256:
`9aca1695e6409668d9627ce865ad8008119f1c9e42d81a6106369f5a95e1872f`.

## Recovered continuation

| Original branch | Result |
| --- | --- |
| 9ED0/A1D8 | Unless flags100000 suppress it, health38 becomes min(maximum64,health+consumed delta), with no lower clamp |
| 9EE4..9F7C/A090 | Flags4000 enable the impulse. Strict XYZ squared norm>64 scales all four impulse lanes by the original refined inverse root and8, then adds impulse times refined reciprocal mass58 times byte7D percent to staged velocity4A0 |
| 9F80..A1D4 | Flags80000 suppress owner-state processing; otherwise snapshot28E0+owner gates597. Compare original position against centers from signed2660/2670 tables plus.5 and radius2780; strict radius>distance writes health-1000 |
| A000/A250 | Nonzero halfword34 with health<=0 calls3510 with attributionFFFFFFFF and optional-request=false |
| A048 | Source body-extension address context+80 becomes relative PC work-context reference80 in5D0 |
| A218 | Flags2 without flags800000 call recovered48C8 adding=true,count_active=true,auxiliary=0; source ignores its result, including unsupported type1 accounting |

Arithmetic retains source operation order, fused operations and round-toward-zero.
No elapsed-time factor, extra health floor, guessed knockback, death/removal policy
or modified owner rule is added. Unwritten context bytes survive.

The5D0 change is an explicit platform adaptation: PC work contexts use an offset
into their own buffer rather than truncating a64-bit host pointer to a32-bit word.
The matching comparison relocates the original work context to local-store0, so
its original context+80 reference is the same80; this does not claim unchanged
host pointer encodings or a matching PC/PS3 executable.

## Counted request construction

The earlier broad label3510 "death path" is narrowed by recovered evidence:
3510 constructs counted kind15 descriptors and clears counter34. It is not an
implementation of the game's complete death lifecycle. The counter's full game
meaning and model interpretation are not guessed.

For counter n, requests are issued in model order72,73,74 with counts n%5,
(n%10)/5,n/10 and scalar values1,5,10. Zero-count requests are skipped. An optional
fourth request uses model76,count1,scalar0. Each source1E090 call receives mode0.
Descriptors copy staged XYZ and velocity, kind15, attribution, marker byte6C=1,
and exact source metadata writes. Reused descriptor bytes remain intact; notably
first-copy C/D clears while second-copy C/D retains its pre-write value. The
counter clears only after the ordered requests. No lower-level1E090 allocation,
queue capacity or scheduling behavior is invented.

Publication is an explicit required sink, with caller-supplied reused descriptor.
A missing sink/descriptor returns MissingEmission without changing context or
statistics. A sink must not throw, re-enter or mutate context, owner statistics or descriptor. All rejecting math/
statistics calculations complete before external requests. The final input data
and consumed result are retained, so resume completion with them rather than
consuming the shared effects again.

## Actual simulation boundary

ArenaConstructionSimulation::complete_player_craft_preparation checks registered
player/root/body linkage and snapshot extents, then completes the staged context
against that owner's actual statistics. The connected sequence is creation,
staging, reserved accumulator consumption, preparation completion, then future
10290 control and already-available6578 finalization. Tests exercise that sequence
using a real shared record and successful conditional clear. Preparation does
not publish movement into world banks or silently enable the F3 preview.

## Validation and remaining limits

A private pinned interpreter atd3ed1a5c946a9c5370b51631e13371a1adf70396 executed
untouched original9A70,1AD8 and3510 instructions.10,000 randomized type1 cases
matched the complete1664-byte context and all counted emission requests. This
includes cap/suppression/owner gates, negative health, random16-bit counters and
reused bytes. The accumulator in this full-routine comparison uses the original
no-target cached branch; source reservation/retry paths were separately compared
in PR #61.1E090 is intercepted at the explicit publication boundary. Reused
emission scratch is supplied from the same original case and compared as an
input boundary, rather than inventing original uninitialized stack bytes.

Eight new public full-context fingerprints use deterministic synthetic inputs,
including cap threshold64, suppression and owner gates. Tests cover missing
emitter/resumption, ordered counted requests, optional request, metadata reuse,
registered-body/reserved-effect integration and rejected foreign owners. Focused
source review found no material issue.

This is scoped output-byte matching, not matching compilation or original PS3
hardware proof. Nonfinite/subnormal/overflow arithmetic and zero-distance refined
root are explicit NeedsNumericRecovery boundaries; zero mass is unsupported.
Source snapshot production, actual1E090 entity publication, original reservation/
job scheduling and full controller/actions/carry remain open. No approximate
replacement is silently used for those dependencies.

## Publication continuation

[Original1E090/2AD8 publication](original-request-publication.md) now supplies the
required sink using caller-owned native FIFO/pool storage. Queue bootstrap,
downstream consumption and original scheduling remain open. The callback is no
longer an unrecovered publication algorithm; its ownership/binding stays explicit.
