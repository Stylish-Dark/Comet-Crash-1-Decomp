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

Inactive players are ignored. Owners are classified by their recovered route
team. The native viewer's third status line now reports this live result next
to base counters; owner switching changes the perspective.

This is **native preview policy**, not recovered instruction-exact mission
win/loss behaviour. It does not end or pause simulation, play animations,
reset the map, implement rewards, distinguish scenario-specific objectives,
or resolve team/base tables that are only partially configured. It is an
incremental gameplay loop milestone, not a completed playable mission.

CTest `comet_native_mission_outcome_test` covers unconfigured, active,
victory, defeat, draw, invalid perspective and same-team-only cases.

Next: recover original title victory/defeat predicates and mission progression
from exact-title binaries; implement weapons and enemy combat to allow the
outcome to arise from complete battle simulation rather than base arrivals.
