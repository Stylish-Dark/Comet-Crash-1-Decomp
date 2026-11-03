# Original player destruction and reset

Reference: Comet Crash NPEB00142 v1.00; FE5D8 job,12288..1265C and
12774..12A70, reached from10290 after depleted health with BC0. Continuation of
original-player-health-contact.md. PROJECT_PLAN P1/P4/P5/P6 advance within
Partial. G1 and G2 remain unaccepted.

## Recovered connected path

`arena_player_lifecycle` now supplies the previously blocked destruction branch.
The actual registered craft can consume damage through1AD8, complete9A70, run
10290 destruction, update its real owner statistics and commit through6578 to
both entity banks and the renderer-facing scene.

| Original source | Behavior retained |
| --- | --- |
|12288..1231C|Set BC1, clear B0, clear BE, set target1E toFFFF, then emit sound7 with original job-cell X/Z.|
|12320..1252C|Three ordered1E880 particle calls:32 model40,16 model43 and16 model48. Preserve reused configuration bytes13..15, complete position/W and original velocity, radii, speeds, lifetimes, acceleration and flags. Execute1F030 immediately before the third call, after earlier publishers consume their RNG.|
|12530..1265C|Signed negative2770 owner base value selects no-base death. BF58 applies the enabled-27B8 base-counter delta-50, then BB30 decrements byte92. A present base with28E0 set also removes the ship, without the base penalty.|
|12774..12A40|A present base with28E0 clear resets590, sets health1 and countdown C0=2, clears74/78 and stage bits, copies cached552/562 to D0/D2, and increments statistics84 through BF58. Its committed statistics116 becomes body D4; BB30 applies2-minus-that-value.|
|10B48 onward|Continue the original banking/proxy lifecycle and delayed position/orientation exchange. Permanent death retires all existing interaction proxy identities through182C0/3400; reset remains a live type1 entity.|
|6578/6704/671C|Permanent removal becomes the original type63 tombstone with frame+3. Both banks and cell markers commit. The projected ship disappears immediately; actual tombstone storage remains until the separately owned release phase.|

BF58 and BB30 reload the full fresh128-byte owner record after each reservation
loss, retaining unrelated bytes. The reset delta is calculated from the committed
BF58 result; a later BB30 reservation loss does not recalculate that delta. This
preserves concurrent changes rather than overwriting statistics116 with a guessed
constant. Scalar arithmetic uses the existing source rounding/clamp semantics.

## Native ownership and failure boundaries

The simulation adapter uses its actual serially owned player statistics record.
Raw10290 additionally exposes the source EA reservation transport, with the
original aligned snapshot2800 base and owner offset. Original job-cell ownership
is required for destruction; missing input returns NeedsJobCell before effects.
Missing particle/RNG, audio or statistics transports also reject before mutations.

All potentially rejecting pure controller calculations precede external effects.
Publication follows original order: pending request, peer effects, attachment and
action requests, destruction sound/particles/statistics, proxies, full-action
particles and owner request. Callback transports must complete synchronously,
without throwing, reentry or inspecting controller inputs. Unavailable/invalid
records after publication begins are fatal host failures, not a new game branch;
never replay the controller after such a failure.

If bank/spatial/display finalization refuses after effects, the existing
NeedsFinalization contract retains advanced context and commit information.
Resume the finalizer, never10290. Tombstone is now a successful terminal result;
both the combined adapter and explicit finalizer refresh the scene. No player
registry clearing or alternative respawn policy has been invented.

## Evidence

- Private original-instruction execution:72 destruction-branch contexts and144
  complete10290 calls match every byte of the0x680 context, all128 statistics
  bytes, three full particle requests, audio arguments and next RNG draw.
  Cases cover four owners, no base/reset/base-with-reset-disabled, both global
  counter-enable values, statistics116 values2/5/15 and both movement modes.
  The complete calls include actual original1F030/RNG, BF58/BB30 arithmetic,
  banking and delayed exchanges. Native platform boundaries intercept audio,
  particle publication and statistics reservation I/O; banking pool allocation
  returns-1 in these comparisons. These are interpreter comparisons, not original
  hardware or compiler-matching claims.
- Public `arena_player_lifecycle.cpp`: actual registered craft, reserved damage
  consumption/clearing, reset and permanent removal through real statistics,
  both banks, job-cell spatial selection and scene. Actual recovered particle
  publisher runs with successful and exhausted pools. Next preparation/controller
  update retains the reset without repeating effects/accounting.
- Reservation losses independently change116 and unrelated bytes during BF58
  and BB30. Fresh retries preserve both; reset D4 uses BF58's committed value.
- Missing transports/job ownership reject without effects. Banking destruction
  publishes four identity-preserving proxy tombstones without allocations.
  Blocked health display resumes through the explicit finalizer without repeated
  effects/statistics; explicit permanent finalization also removes projection, remapping structure-cell links after vector compaction.
- Focused Release checks: lifecycle, controller, entity finalization, statistics
  and particle publication pass. Independent source review identified and fixed
  missing job-cell ownership and explicit-finalizer scene refresh. Connected
  execution additionally exposed and fixed retained live rendering of tombstones.

No proprietary code, original binaries, assets, disassembly or private comparison
harness is committed.

## Remaining work

Ship pickup/link and voluntary drop, hostile capture and excluded numeric paths,
original snapshot/bootstrap/scheduling and complete input integration remain.
The supplied ready BC0 and job snapshot in fixtures are original-state inputs;
they do not recover the missing outer bootstrap. The reset's next update is
connected, but a full autonomous return-to-ready loop still needs original base
selection/attachment scheduling. Particle/audio requests are not complete native
original effects/audio presentation. Campaign, AI, content and persistence remain
substantial. The playable preview remains separate; this checkpoint changes the
recovered original path, not its preview rules or downloadable build.

Overall estimate remains about25%, plausible15–35%, toward the complete original
game faithfully recovered, integrated and usable on Windows with mouse-capable
controls. This closes a real controller branch, but does not accept a full player
loop or mission or justify a five-point overall estimate increase.
