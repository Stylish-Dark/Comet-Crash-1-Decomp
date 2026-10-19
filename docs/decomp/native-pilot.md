# Opt-in native pilot preview

In a construction/skirmish viewer, press **F3** to switch between grid controls
and the pilot preview. **P/Start** runs or pauses. While playing, the left stick
or **I/J/K/L** flies the displayed player ship; the yellow build cell follows it.
Existing build/type/factory/release actions operate on that cell. F3 requires a
human owner and loaded ship model (original model slot 0). The default remains
grid mode, so existing keyboard and controller workflows are unchanged.

The ship is a control/render cursor using the [recovered free-flight step](native-craft-flight.md), not a registered combat entity.
It cannot take damage, carry resources or deploy from its position. Ground-unit
release still originates at the configured base. The selected position/velocity arithmetic is now recovered; collision/actions and
the completed original mission remain open.

## Original evidence and portable boundary

Original ELF SHA-256:
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
Pad decoding stores left axes at player +1F8/+1FC (pad subobject +150 plus A8/AC).
In the PPU `F2350` input consumer:

- `F2DB8/F2DBC` read these two floats and stage `(X,0,Z)`.
- `F2DF4..F2E34` calculate the vector's magnitude using VMX square-root estimates
  and refinement.
- `F2E38` compares it to 1; values at or below 1 bypass normalization.
- `F2E40..F2E70` normalize vectors longer than 1 using reciprocal estimates and
  refinement.
- `F2E8C/F2E90` store the resulting Z/X commands at player +14/+10.

`arena_player_stick` recovers this rule: preserve sub-unit analog strength and
cap diagonals to unit length. It uses portable `hypot`/division instead of VMX
estimate/refinement, so **no bit-exact VMX or original-instruction comparison is
claimed for this leaf**. The preceding scalar byte/deadzone decoder retains its
existing exact 256-input fixture. Further downstream consumers are still open;
this evidence alone does not establish original acceleration, speed or collision.

## Explicit native preview policy

- Recovered mode-flag1 free flight: target speed10, response4.6*elapsed; frame
  input delta capped at .25 seconds. The mode selection is native adapter policy.
- Position clamped to cell-center bounds .5 through extent-.5, fixed Y .85.
- Heading follows velocity; no collision. Acceleration/deceleration and the
  old-velocity position order use the recovered step.
- Keyboard direction takes precedence over analog input while a pilot key is held.
- Movement runs only when focused, playing, unfinished and human-controlled.
- Pause/focus loss/disconnect/cancel clear held keys and require neutral pad input
  before controller rearming. Owner changes and F2 restart reset the ship to the
  selected human base. F3 starts it at the current inspection cell.
- Arrow cell navigation is disabled in pilot mode; use F3 grid mode to inspect.
- The existing model loader applies geometry scale; rendering adds no second scale.

## Validation and remaining work

The CPU test covers all 65,536 pairs of decoded byte-axis values, preservation
inside the unit circle, unit-length diagonal cap, inertial start/stop, disabled input,
bounds, unit orientation, reset and invalid-input rollback. Existing gamepad edge,
neutral-rearm and exact scalar-axis tests continue to pass.
The offscreen SDL test switches F3, flies a virtual controller to the east edge,
pauses, rejects held-stick/arrow movement and builds a factory in the ship's cell.
A separate run flies out then restarts and verifies the occupied base cursor
rejects building without consuming funds or advancing the scene. Existing phases test grid controls,
production/release, focus loss, unplug and held restart. It uses synthetic model
geometry; physical-controller and fresh original-asset render validation remain
open. Fixed-duration synthetic input pulses remain a slow-host test limitation.

Original craft collision/action gates, aircraft, richer AI, authentic economy,
remaining weapons, mission/campaign rules and HUD/audio remain open. See
[native-craft-flight.md](native-craft-flight.md) for the current recovery boundary,
original-instruction comparison and capability estimate.
