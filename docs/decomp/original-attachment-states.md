# Original expired attachment states and audio batches

Checkpoint2026-10-27, above PR #69. P1/P2/P5/P6 advance within Partial.
G1/G2 remain unaccepted. Complete faithful PC-port estimate stays about25%,
plausible15–35%; this is not measured instruction coverage.

## Connected original continuation

10290 no longer rejects11DE8 solely because the attachment countdown expired.
Both original BC branches now execute with their supplied attachment/cache state:

| Source | Recovered behavior |
| --- | --- |
| 11E08..11F6C, BC nonzero | Exact-zero74 queues sound8. Progress becomes fma(elapsed,.5,old74); stage6/model0,78, health=progress*64 and zero velocity follow. Position interpolates fetched Y toward1, retaining packed W. Strict progress>1 clamps to1 and clearsBC. |
| 12660..12770 | When progress crosses1 and D4!=2, emit the original reused kind16/model108 descriptor through1E090. Position, craft identity, owner, D4/2 scalars and untouched bytes match source. This is a request, not a fabricated world mutation. |
| 11F70..12194, BC zero | If health<maximum, inspect up to4 original players: same2720 group and signed16 wrapped X/Z differences in[-1,1]. Eligible proximity adds maximum*(elapsed*.1), clamps to maximum and emits the original model37 particle request. Ineligible states retain ordinary integrated movement. |
| 3940/3994..39EC | Nonzero body7F returns immediately. Later health changes can commit through6578 for an already-initialized health-display state. First-display7F0 allocation remains explicit. |

B588's128-byte audio packet is now a readable native primitive: count at0,
28 entries at4, each containing a16-bit sound and two parameter bytes. Full
packets remain byte-for-byte unchanged but still attempt the conditional write.
A9F8/A9B0 reservation loss reloads a fresh packet and retries the append; no stale
snapshot is appended twice. Missing platform transport remains explicit. This
queues original sound events; audio playback/queue consumption is not recovered.

Controller callback order remains pending3238, attachment audio/completion/repair,
action visuals, interaction proxies, full-action particles and owner request.
All rejecting computations precede callbacks. Audio/particle sinks obey the same
no-throw/no-reentry/no-controller-observation contract. The expiredBC0 path retains
its observable integrated packed W; other attachment paths replace that lane.

## Evidence and integration limits

24 targeted original10290 cases match the complete0x680 context, all proxy bodies,
ordered sound/counted-request/particle payloads and caller RNG continuation.
These cover four owners, growth onset/progress/completion, D4 gates, prior packed
W and nearby/far/same-group repair. Source6578 ->3940 then matches all256 final
body bytes for those24 cases with an existing-display flag. Particle callbacks
are captured at1E880 here; actual particle generation/publication was established
in #69, not re-counted as new work.16 originalB588 cases independently match all
128 packet bytes and every conditional-write attempt across append/full/retry.

Public tests preserve the original audio fingerprint, fresh-reservation retry,
missing-sink atomicity, completion request and repair predicates. Nine consecutive
registered-scene updates run the source growth sequence from progress0 to1 and
its strict completion crossing, with one sound, one completion request, eight arm
particles, health commits and both authoritative banks/render positions retained.
The initial attachment/cache and existing health display are explicit source-state
fixtures. No pickup/drop or health-display creation is inferred from those fixtures.

Release build and all75 CTest cases pass. Five restored build-artifact launch
issues (four lost executable modes and one empty file) were relinked; only their
failed launches were rerun. Repository safety/whitespace checks pass. Independent
source review confirmed state gates, callback order, B588 retry and3940 early return.
These are scoped semantic comparisons, not hardware/compiler/full-game parity.

Next: recover first3940 health-display creation (2AD8 count2/reserve1024;
type1 models105/106), actual pickup/drop/held-unit behavior, real-peer contacts,
12288 death, entity pending transport, scheduler and original command/mouse input.
The complete original mission and running original player loop remain unaccepted.
