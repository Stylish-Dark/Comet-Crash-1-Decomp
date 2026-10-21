# Original entity preparation: state staging

Recovered FE5D8 SPU9A70..9EC0, stopping immediately before the9EC4 call to1AD8.
Type1 wrapper12B88 reaches this preparation before10290 and6578. This closes
the shared state-staging dependency in front of the player controller; it is
not a claim that all of9A70 or the controller is recovered.

Reference EBOOT SHA256:
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
Job SHA256:
`9aca1695e6409668d9627ce865ad8008119f1c9e42d81a6106369f5a95e1872f`.
Private reference bytes were restored from the owner's existing uploaded game
archive after workspace cleanup; no reference binaries or instruction dumps
are committed.

## Recovered data flow

| Source context | Recovered writes |
| --- | --- |
| +480/+4A0 | Copy persisted body position/velocity, including packed fourth lanes |
| +490 | Copy position and clear only Y; retain X/Z and packed W |
| +4B0 | Original ordered quaternion rotation of forward(0,0,-1,0), fused arithmetic and four lanes |
| +4E0/+4E4/+4E8 | Original byte-derived radius/lateral radius and previous health bits |
| +4F0/+500 | Entity identity/kind extraction, including secondary packed lanes |
| +510/+520 | Stage and stage-bit vectors, including secondary packed lanes |
| +542/+544 | Both halfwords receive the unpacked owner; stage lives at+510 |
| +530 | Four visibility halfwords unpacked from the selected snapshot player block |
| +552/+562 | X/Z truncate toward zero and clamp to original extents; grid stride remains24 |
| +595 | Original tile byte bit test, six-bit shift count and flag80000 suppression |
| +570/+590..+59A/+5A0 | Original100 scalar, specified byte resets and initial state word1 |
| +4C0/+4D0 | Copy original snapshot2710 vector and zero the other vector |

Every unmodified byte survives. The body and accumulator region remain unchanged
by this boundary. Snapshot policy is supplied explicitly, not synthesized from
preview rules. ArenaPreparationSnapshot is the root image required by the source:
16-byte tile records,2710 vector,2720 player halfwords and28D8/28DC extents.

ArenaConstructionSimulation::stage_player_craft_preparation loads the actual
registered craft body from the current read bank and stages it into caller-owned
reused work context. It checks player/root linkage and matching snapshot/world
extents. It does not change the world, resources or pending effects. No F3
replacement-control path is silently enabled.

## Validation and limits

Private comparison executes untouched instructions with the pinned ps3recomp
interpreter atd3ed1a5c946a9c5370b51631e13371a1adf70396, starting9A70 and stopping
before9EC4.10,000 deterministic randomized cases matched all16,640,000 output
context bytes, including arbitrary reused context bytes, packed secondary lanes,
orientation, grid clamping, owner visibility and flag gating. Eight public
full-context fingerprints use deterministic synthetic inputs, with no proprietary
instruction/data bytes. Tests also exercise registered-body integration and
transactional rejection. Release native build and all68 headless tests passed,
with local launch permissions repaired and failed launches retried.

This is output-byte comparison for this boundary, not matching compilation or
whole-game/hardware parity. Inputs support owner0..3 and extents1..24, normal/zero
XYZ/quaternion values bounded by1e6. Nonfinite/subnormal arithmetic intermediates
are rejected without writes; the host interpreter does not establish original
SPU extended-exponent or underflow behavior. The rounding mode is restored.

## Connected accumulator consumption

The next routine1AD8..2050 is now recovered in arena_entity_accumulator.cpp,
including its call from preparation's workcontext+300. It returns the complete
health quadword+10, impulse quadword+40 and request word+30. The caller's request
output vector only changes its first word; remaining words are preserved.

The source pending gate checks nonzero health, ordered XYZ impulse squared norm,
positive signed counters50/70, nonzero raw words4/8/60 and78 !=FFFFFFFF, with a
nonzero target at0. An idle/no-target local snapshot returns cached values without
platform access. Otherwise it reserves a FRESH shared128-byte snapshot, clears
exact source lanes, performs the conditional write, and retries from a new
reservation on failure. Successful consumption restores the fresh snapshot in
local memory so the controller reads the values actually consumed.

| Shared record | Clear operation |
| --- | --- |
| 10/20/40/50 | Entire16-byte regions zero |
| 30 | Entire16-byte region FF |
| 0 | Preserve original transaction target supplied by local record |
| 4/8/60/70 | Zero words |
| C..F | Zero bytes |
| 74 | Float100 bits42C80000 |
| 78 | FFFFFFFF |
| All other bytes | Preserve fresh shared snapshot |

GETLLAR/PUTLLC are explicit required platform reservation callbacks, corresponding
to A9F8/A9B0. The implementation does not fake successful DMA or fabricate a zero
accumulator. Retry reads fresh values; contention cannot cause stale effects to
be applied twice. Missing/unavailable transport returns an explicit status and
preserves the local record. The context wrapper only changes300..37F after
successful consumption. Native binding of these source records/atomic ownership
into the automatic world scheduler remains open. Existing preview health vectors
are not silently promoted into the complete original accumulator.

A second private comparison ran untouched1AD8..2050 instructions with synchronous
reservation transport and explicit0..3 failed conditional writes.10,000 cases
matched local128 + shared128 + outputs48 bytes, including no-target and idle
branches. Eight public synthetic-input full-result fingerprints cover this
boundary. Release build and all69 headless CTests passed. Source review found no mapping
issue. Numeric support is finite
normal/zero health and impulse XYZ and normal/zero finite norm intermediates;
this is not SPU extended-exponent/hardware matching. Other returned lanes remain
raw bits. PS3 concurrent DMA scheduling is not claimed by synchronous comparisons.

## Continuation recovered in the next milestone

See original-preparation-effects.md:9EC8..A268 and3510 counted requests are now
recovered, including the registered-body completion API.3510's earlier broad
"death path" label is corrected there. Full10290 controller/actions/carry, actual
entity publication and source scheduling remain open; numeric/transport limits
remain explicit.
