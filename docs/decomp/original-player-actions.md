# Original player action selection and visual requests

Checkpoint 2026-10-23 (Australia/Sydney), stacked above PR #65 on
`work/original-player-actions-20261023`. Completion-map P2 changes Open -> Partial;
P1/P3 also advance. G1 remains unaccepted.

## Implemented original behavior

The existing10290 mode0 entry-to-commit path now executes10760..10AE8 action
handling rather than returning NeedsCarry. A positive action command with BC0
enters the branch. BB is its edge latch: nonzero BB emits the original four
kind18/model112 requests and becomes0; holding does not emit again. Releasing
(or the original BC gate) rearms BB1 and clears1E/BE/BF as113E0 requires.

11808..119B8/12A70..12B80 emit in this exact order:

| Order | Stored target | Selector at +40 | Scalar at +30 |
| --- | --- | --- | --- |
| 1 | extension c8 | 0 | +1 |
| 2 | extension c8 | 1 | +1 |
| 3 | extension ca | 0 | -1 |
| 4 | extension ca | 1 | -1 |

Each request has count1/mode0, XYZ -100, subtype21, owner metadata and the
original reused/unwritten bytes. The +20 copy retains the pre-clear identity
bytes on the first emission. An independent action descriptor is retained in
ArenaPlayerControlScratch; owner-timer descriptor storage is separate. Pending
allocation callbacks still precede these emissions; owner emissions follow them.
Missing required callbacks or unsupported arithmetic reject without changing
controller/scratch or emitting any request.

107B0..10AE8 now preserves source candidate selection: a non-sentinel1E candidate
of kind<=60 within strict distance3 sets BF1. Original1F560's first return lane
measures candidate distance to the current rotated-forward segment between
half and full forward displacement. Strict distance<.5 sets BE1; an eligible
miss preserves the previous BE byte. Ineligible candidates clear BE/BF and take
the fallback identity at378. The original candidate identity is retained when
eligible. No new target search or range policy is substituted.

106B0..10750 target-facing is recovered too, so a selection can advance on the
following update. It normalizes the original candidate displacement, applies
original .9 * elapsed * 4pi yaw step through the recovered1A118 aim, and preserves
the source delayed visible/pending quaternion exchange. Existing near-axis and
finite-normal numeric boundaries remain explicit. This is not banking or automatic
control. Stack-only gripper/quaternion results with no observable use in mode0
are elided; no equivalent elision is claimed for mode1.

## Integration and validation

The same simulation and scene APIs now execute these branches into registered
craft banks and renderer-facing state, using the original1E090 publication pool
for emitted requests. A four-update press/hold/release/repress test verifies actual
published records, preserved latch state and both committed banks. Focused tests
also cover candidate selection -> target-facing continuation -> button-release reset,
ineligible target fallback, missing-emitter transactional rejection, request
sequence/metadata, and preserved reused bytes. BC0 and candidate/context images
are explicit test inputs; they are not asserted to be original mission bootstrap
or recovered peer-cache scheduling.

Two private original-instruction comparison suites each exercise5,000 complete
10290 paths with all four owners, randomized contexts/reused scratch, pending
allocation success/failure, owner timers, candidate kinds/ranges/fallbacks and
button/edge states. The ordinary-facing suite has4,993 accepted exact cases,
6 matching contact-effect boundaries and1 explicit near-axis exclusion. The
mixed ordinary/target-facing suite has4,994 accepted exact cases and6 matching
contact-effect boundaries;361 cases retain selected targets. Every accepted case
matches the entire0x680 context,256-byte pending scratch, independent128-byte
owner/action scratch, all captured request/reserve/publish payload bytes and
callback counts. The mixed suite makes2,455 reserve attempts,1,856 publications
and2,521 counted request emissions. Some inputs overlap across suites; do not
report their sum as10,000 distinct cases.

Original2058/2198/1E090 remain explicit comparison transport boundaries;
10290 and the called source math execute in the private pinned interpreter.
The public scene integration separately uses the recovered1E090 implementation.
No original compiler byte-match, hardware parity or whole-game fidelity claim
follows from these comparisons. Original binaries/oracles remain private.

Focused source review found no material issues in the final action/facing scope.
The build/test checkpoint is recorded in STATE.md; do not infer acceptance from
this document alone.

## What remains and next work

Correction from the next source checkpoint: FDFE0 initialization maps kind18 to
2BE0/5558. That consumer reads player/target bodies and updates its own visual
record; it does not detach or mutate the target body. The earlier “release
requests” label and proposed carry-consumer dependency were incorrect. The
controller/request comparison evidence above remains valid for the emitted bytes.
See [reference fetch and attached position](original-player-references.md).

Actual pickup/link creation, drop/unlink, interrupted/death cleanup and ground
entity carrying updates remain missing. The ordinary attached-position branch is
now recovered in the next checkpoint, but its test explicitly installs the link;
it does not demonstrate pickup. Automatic control/banking, energetic contacts,
death/change effects, snapshot/job ordering and input adaptation remain open.
P1 is still Partial; P2 now has recovered controller decisions/publication but
still lacks its connected world-change acceptance criterion. G1/G2 remain open.

The provisional G1 remaining allowance stays16/32/64 focused implementation hours
(low/likely/high), low confidence and uncalibrated. This checkpoint closes the
ordinary action dispatch and target-facing entries in the controller branch list;
it does not establish throughput for the missing consumers or give a whole-game
deadline. No reliable phase timing was recorded, so none is reconstructed.
Overall full-game planning estimate remains about25%, plausible15–35%.
