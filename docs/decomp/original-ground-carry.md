# Original carried ground-unit continuation

**Continuation:** [original linked motion and timeout](original-linked-ground-motion.md)
now removes this document's supplied14060-motion boundary for the supported
linked stage5 path. Historical comparison scope below stays unchanged.


Reference: NPEB00142 v1.00 ELF SHA256
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`,
FE5D8 job SHA256
`9aca1695e6409668d9627ce865ad8008119f1c9e42d81a6106369f5a95e1872f`.
Addresses are job-local. Original binaries, disassembly and private comparison
programs are not distributed.

## Recovered state and caller

This is the kind47 ground carrier loading ground units, distinct from the
player craft’s interaction-arm/structure pickup path.

| Source | Native behavior |
| --- | --- |
|12CD0 ->3108,12D00..12DAC|Actual read-bank references: self accumulator300, peers100/200, signed B4 parentA00 and B6 secondaryB00. FFFF retains reused cache bytes. Original tag59C is explicit.|
|14538..147FC /14AFC|Parent-relative formation slot, target-player inheritance for the matching team, quaternion-rotated X/Z offsets and correction capped at1.5×elapsed. B2 is the signed formation slot; **B4 is the parent**.|
|158B8 ->16A50..16DE4|Common flag800 clear and body18 approach to1, model124/stage inheritance, horizontal drag, conditional vertical spring, velocity cap.5, bank adjustment and shortest-arc orientation blend. Norm refinement preserves each lane's cyclic sum and rounding.|
|17EE8|Dead/retired parentkind>=62 clears flag1000 and B4/B6, sets B8=1,8C=2 and stage5. This is parent-loss unlink, not voluntary release.|
|15FA0 ->16878 ->14F8|Contact-driven carrier request:1000 carrier,2000 peer, strict gap<.4 and no pending200000. Claims command2; success decrements B0 capacity, clears1000 when full and sets200000 pending. Payload carries real parent/peer identities and formation slot20−B0.|
|15FF0 ->16710 ->175B0 ->14F8|Incoming command2 claims command3 on the parent accumulator. Success clears2000 and writes B4=payload364, B2=payload368; B6 is unchanged. Occupied leaves links unchanged, while local opcode still becomes3.|
|17000 ->B588 /16060|Command3 clears200000 and emits original audio60 with job-cell parameters. Commands2/3 skip BC updates; ordinary tail resets BC3. C4 timer decreases; unrecovered effect branches remain explicit.|
|6F40 ->7174 ->6578 ->6568/6260|Alive/no-base-event gate, existing health-display finalization, nearest-peer selection and publication into both owned banks/cell lists. Original frame and job-cell inputs are explicit.|

`ArenaConstructionSimulation::advance_prepared_ground_carry` and its scene
adapter commit the actual unit record and project its position/orientation from
the authoritative bank. They take **completed14060 output**, not a render-frame
steering approximation. `stage_ground_carry_references` fetches real bank data.
`complete_prepared_ground_carry_command` connects the recovered request/claim/tail
and finalizer; after externally publishing a claim, a bank/effect rejection retains
advanced work. A bank/finalization rejection resumes through
`finalize_prepared_ground_carry`; a boundary within the command tail must first
complete that missing continuation. Do not bypass it or replay a published request
or acknowledgement audio. Basic invalid elapsed/C4 inputs reject before any claim. Native reservation loss rereads fresh128-byte
records; even an occupied slot requires successful conditional write confirmation.
The integration now uses an actual kind47 carrier and kind40 passenger. It
publishes command2 into the passenger accumulator, consumes it through original1AD8,
claims the parent acknowledgement, writes B4/B2, consumes command3 on the carrier,
emits audio60 and commits the records into both banks/scene. An additional sequence
migrates a linked unit across cells, fetches an actual registered parent tombstone
and executes17EE8. Preceding ordinary movement/contact output remains supplied;
this does not claim a complete18030/14060 wrapper or a player-controlled pickup.

## Shared nearest-peer selection

Recovered6568/6260 scans the original fixed3×3 job neighborhood, X outer/Z inner.
The job's88/8A halfwords initialize20DC0/20DD0; movement does not choose a new
search origin. Packed entry0..1 is identity,2 radius hundredths,4..6 localXYZ,
and(byte7>>2)&7 is category. Body10 is the category mask and4F0 the excluded own
identity. Cell culling uses horizontal center distance and own radius4E0. Actual
candidates use 3D surface distance. Strict improvement from100 preserves scan order
on ties. The function writes body1C and the full SIMD score quadword5B0.

The aligned16-byte load matters: an odd-index record rotates beside the preceding
record; it does not read the following record for its nonpreferred lanes. For an
odd count, unused eight bytes after the last record remain explicit `tail` input.
The native vector-backed bank view initializes this unused padding to zero; the
raw source-context API accepts exact reused bytes. This platform storage choice
affects nonpreferred scratch lanes, not the preferred peer identity/distance.

**Correction to previous finalizer notes:** original r4=0 skips this spatial pass;
r4!=0 runs it. The old claim was reversed. The existing explicit
`skip_spatial_6260` native boundary is retained for callers using the record-only
finalizer. The registered craft controller/scene adapters now also execute6260
when given `original_job_cell`; callers omitting it retain their older partial
boundary. The four-step registered-craft fixture exercises the new continuation.

## Evidence and remaining boundaries

Private interpreter comparison with the pinned SDK commit
`d3ed1a5c946a9c5370b51631e13371a1adf70396`:

- 1,350 fullC00 contexts for formation/dynamics: kinds40–48, parent1/62/63,
 signed slots-3/0/4/5/19, elapsed.016/.25 and positive/negative capped/uncapped speeds.
- 64 full contexts for6568/6260: nine populated cells, different category masks,
 per-lane radii/identities, packed W metadata and explicit reused trailing bytes.
- 45 full contexts for16710 ->175B0 ->14F8 ->16060 and45 for the contact
  request15FA0 ->16878 ->14F8: success, occupied, fresh retry changing occupancy,
  no-request gate, signed payload halves, slots and accumulator addresses.
- 9 command3 contexts and audio-event arguments match at the B588 boundary.
- 24 full contexts for158B8 ->6F40 ->6578 ->6260, including live and lost parents,
 compared against the real packed bank neighborhood before publication.

Public fixtures retain source-derived bit outputs, bank/cell/scene progression,
parent-loss unlink and atomic refusal of an unrecovered base-event branch.
Independent review checked reference offsets, entry writes, special-kind guards,
base gates and spatial decoding. These comparisons establish their stated domains,
not original hardware parity or a complete original mission.

Still missing: player craft pickup/link/voluntary release, ground-carrier unloading, the rest of14060
(target acquisition, contacts, integration/bounds and steering), special update
branches595/596/8C and kind45/47/48, other effects/commands, base/death handlers, and original
scheduler/bootstrap/control-to-mouse integration. Formation and late dynamics are
separate source entrypoints because the missing14060 middle must not be omitted.
Finite normal/zero numeric domains are bounded; degenerate2590 bank bases and
zero-distance estimate boundaries return NeedsNumericRecovery. SPU extended
exponents/subnormal/overflow behavior remains unresolved. No guessed numeric
fallback or original elapsed scheduler is supplied.

P1/P2/P3/P6 remain Partial. G1/G2 are not accepted. Overall faithful PC-port
estimate remains about25%, plausible15–35%, for the entire original game usable
on PC with mouse controls; this checkpoint does not justify revising it.
