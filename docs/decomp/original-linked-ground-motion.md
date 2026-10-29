# Original carried-unit motion and failed pickup recovery

Continues PR73 (`a5ce7bd2a2e1337bc7d9ace0036a61002990bda3`). P2/P3/P6
advance within Partial. The older carry adapter required externally supplied
completed14060 movement. The new linked adapter executes that movement itself
for the supported stage5 carried path, then158B8 and6F40/6578/6260, committing
both original entity banks and the renderer-facing scene. This concerns ground
carrier/passenger behavior, not player-ship grab/drop or an accepted mission.

## Recovered connected path

| Original source | Native behavior |
| --- | --- |
|14090..14138,14AD0,12BE0/3068/2CE0|Retain a live target; otherwise try owner257E override, then ascending hostile-team2578 players with live2770 base references. Choose with one shuffled RNG draw. No candidates means target255 and no draw. Owner/team/base snapshot initialization remains explicit.|
|14138..14188,14538|Use the fetched real B4 parent and signed B2 formation slot, preserving target inheritance and quaternion/packed-W arithmetic.|
|14188 ->7E40/7510|The same single-peer r6=1/r7=1 call as the player controller, now also admitted for ground kinds40..48. Preserve source geometry/contact/RNG, deferred16F8 impulse and1E880 particles. Hostile kind45 capture is still an explicit boundary.|
|141A0..14228,14528/7DD0|Integrate source velocity after formation/contact. Linked B4 bypasses local terrain20 and uses original world bounds. BA records **any plane contact**, including gentle contacts; energetic impact alone is not that flag.|
|14250..14524|Update clamped source cell552/562; move old CC/CE to C8/CA only on cell change; set574 to32.|
|14320..14420,13998|Execute source routing/direct/blocked behavior and height correction. No-target14450 retains original rotated-forward horizontal steering. Blocked596 and target598 retain their source roles.|
|16008..16060,171F0..1725C|An ordinary tail with BC1 and pending bit21 decrements BC to0, restores one wrapping B0 slot, clears pending200000 and, for kind47, restores loading1000. Other high bits alone do not trigger this. Commands2/3 bypass timeout; subsequent updates cannot restore the same slot twice.|

The existing `prepare_arena_player_peer_contact` retains its type1 entry contract.
A shared internal implementation also serves `prepare_arena_ground_peer_contact`;
this adds ground callers without altering player call arguments or effect order.

## Owned integration and resumption

`advance_prepared_linked_ground_carry` in the simulation and scene takes completed
shared9A70 state and fetched12CD0 references. It executes motion and linked
158B8 dynamics, then publishes peer impulse before particles and finalizes the
advanced state. All rejecting calculations and missing-transport checks precede
external effects. Complete transports share the original player adapter's
no-throw/no-reentry/no-input-inspection contract; a fatal transport failure is not
resumable by replaying the update.

Every finalization refusal maps to `NeedsFinalization`. Completed work and RNG
remain retained, including already published effects. Resume through
`finalize_prepared_ground_carry`; never rerun motion or republish contact effects.
This distinguishes post-effect failure from transactional pre-effect rejection.

The connected native scenario creates an actual carrier47 and passenger40,
requests pickup through their actual accumulators, claims B4/B2 and acknowledges
command3. Eight subsequent updates fetch the actual committed bodies/parent,
execute shared9A70 and recovered linked motion, and commit the moving passenger
to both banks and the scene. These updates supply no fabricated completed14060
position or velocity. Separate failure/resume coverage publishes one real peer
impulse, refuses finalization, repairs finalization input/display state and commits
without publishing the impulse again. The test's snapshots, setup and native
serial ownership remain explicit fixtures, not original mission bootstrap.

## Evidence and boundaries

Private pinned FE5D8 instruction comparisons match all C00 context bytes and
the next RNG draw for1,296 linked motion contexts: three ground kinds, three
velocities, direct/ordinary routing, blocked/unblocked routes, two elapsed steps,
no/stale/live peers and existing/random/forced/no-target acquisition.24 additional
edge/cell-suppression/packed-W contexts match after correcting BA.50 complete
command-tail contexts match source for pending/other high bits, counter states
and carrier/noncarrier kinds. Counts are evidence of this input domain, not
whole-game coverage. Original code, assets, disassembly and comparison tools
remain private and are not distributed in this repository.

Independent source review found the BA mistake and ambiguous finalization status;
both were corrected. Public focused scenarios cover those fixes, timeout
non-repetition, command acknowledgement bypass, target draw counts and connected
bank/scene commits. The player contact and controller checks cover the shared
peer adaptation. Build/run the focused evidence:

```sh
cmake -S decomp -B build/native -DCMAKE_BUILD_TYPE=Release
cmake --build build/native --target comet_arena_ground_carry_test comet_arena_player_peer_test comet_arena_player_control_test
ctest --test-dir build/native -R 'comet_arena_(ground_carry|player_peer|player_control)_test' --output-on-failure
```

Still missing: full ordinary/unlinked14060 and non-stage5 approach branches,
hostile capture continuation, energetic world-hit particle handling in this
adapter, special158B8 command/effect branches and ground-carrier unloading,
original scheduler/bootstrap and runtime input integration. Dead-parent14060
ordinary continuation still refuses here; the older explicit late158B8 parent-loss
adapter remains available. Square arena extents1..24, finite normal/zero scalar
math and documented packed-W lanes are supported. SPU subnormal/extended-exponent
and zero-estimate exclusions remain. No original hardware/compiler match or
complete player loop follows from these comparisons.

G1/G2 stay unaccepted. The delivered preview's ship pickup and original campaign
are not enabled by this checkpoint. The coarse complete-faithful-PC-game estimate
remains about25% (plausible15–35%); remaining campaign/AI/lifecycle/presentation
work dominates, and this scoped connected recovery does not justify a new rounded
whole-game estimate.
