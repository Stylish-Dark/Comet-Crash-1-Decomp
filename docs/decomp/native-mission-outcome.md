# Native mission outcome preview

The native interactive scene now derives a player's mission outcome from the
simulation's **live base counters**, including counters decremented by ordinary
ground-unit base impacts.

`evaluate_native_mission(simulation, perspective_owner)` returns:
- `Unconfigured` when an invalid owner or either active opposing side has no
  configured base counter;
- `InProgress` while both friendly and enemy teams have a positive counter;
- `Victory` when at least one friendly base survives and no configured enemy
  base survives;
- `Defeat` for the inverse;
- `Draw` if both sides have no surviving base.

Any missing counter for an active player makes the table unconfigured, even
when another counter exists on that player's team. An inactive perspective is
unconfigured. Inactive players otherwise do not participate. Owners are classified by their recovered route
team. The native viewer's third status line now reports this live result next
to base counters; owner switching changes the perspective.

This is **native preview policy**, not recovered instruction-exact mission
win/loss behaviour. It does not implement original campaign objectives,
animations, rewards or progression.

## Finished matches and restart

`NativeMissionPlayback` freezes the interactive simulation after a tick leaves
one surviving team (victory/defeat) or none (draw). With three or four teams,
one eliminated team does not stop the survivors. Missing-counter and single-team
development scenes continue running without inferred victory. All configured
held deployment strengths become zero on finish. Construction, factory changes,
deployment, manual spawning, motion-mode toggles and further P/T ticks are blocked.
Camera controls and player/type/cursor inspection remain available.

**F2** restores the complete state supplied when the viewer opened and returns
paused: both entity banks, packed cells, free-index FIFO, queues/events, RNG,
funds, quotas, timers, counters, configuration and visible scene. It does not
reread files. Launch-time scripted deployment is restored with the checkpoint;
manual held input is cancelled, owner/type selection is reinitialized, and the
construction cursor/type reset. P resumes or T takes one tick. F2 also works
before a match finishes. R remains the camera reset.

Restart uses a value-owned heap snapshot and restores the scene in place, keeping
the viewer's arena reference valid. This adds replayable preview lifecycle;
combat, enemy AI, original mission rules and full playability remain unfinished.

CTest `comet_native_mission_outcome_test` covers unconfigured, active,
victory, defeat, draw, invalid/inactive perspective, missing active counter,
allied survival and same-team-only cases. Assertions remain active in Release.
`comet_native_mission_playback_test` exercises an actual ground-base impact,
freeze, deployment cancellation, restart, queued construction and RNG replay,
plus three-team continuation and unconfigured scenes. The graphics test sends
real SDL key events to check finished-match guards and F2 restoration.

Next: recover original title victory/defeat predicates and mission progression
from exact-title binaries; implement weapons and enemy combat to allow the
outcome to arise from complete battle simulation rather than base arrivals.
