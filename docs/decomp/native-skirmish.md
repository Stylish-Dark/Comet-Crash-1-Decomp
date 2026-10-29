# Interactive native skirmish preview

Continues PR #51 (`6a28a322`). The native executable can now start a configured
battle without extracting or matching an original campaign map. It uses the
existing recovered simulation, original model/texture loader, SDL/OpenGL viewer,
base-counter outcome and F2 checkpoint. This is a native preview mode; original
campaign/AI recovery remains separate.

## Mouse gameplay

Click a structure button, then an empty ground cell to build. Factory is the
normal starting structure. Click PLAY, then hold HOLD RELEASE to deploy units.
Click an existing factory to select it; its blue outline stays selected while
moving to FACTORY MODE. PREV UNIT/NEXT UNIT choose the release kind. RESTART
restores the launch checkpoint. Right drag orbits and the wheel zooms.

The toolbar forwards the existing construction/deployment/factory APIs. It does
not implement different build costs, combat, AI or mission rules. Keyboard and
controller inputs are retained. The separate ship preview still uses IJKL/stick.

## Launch

The Windows CI package includes `OPEN_SKIRMISH.cmd` and `native-skirmish.cfg`.
Extract it and drag the user's extracted game folder onto `OPEN_SKIRMISH.cmd`.
The launcher accepts the game root, USRDIR or data directory. Original game
assets remain required and are not included in the repository/package.

From a source checkout:

```
comet_native --assets "D:\CometCrash\USRDIR\data" --skirmish-config decomp/examples/native-skirmish.cfg --arena
```

Without `--arena`/`--viewer` or assets, the same config can be validated headlessly:

```
comet_native --skirmish-config decomp/examples/native-skirmish.cfg
```

The supplied 24x24 encounter has human player 0 at (2,12), an automatic opponent
at (21,12), 400 starting resources, eight-unit quotas and ten-hit base counters.
Contacts, Tesla and cannon paths are enabled. The opponent builds its ordinary
factory and releases default kind-40 scouts.

The viewer starts paused, with the build cursor on the selected human's base.
Press Right, 7, B to build a factory in the next cell. Press P to run and hold D
to release produced units. Press 3 and B on an empty nearby cell for a Tesla
that can damage approaching enemies. Arrows move the cursor; Q/E choose release
kind; F5..F9 select a factory's production mode. Base depletion ends and freezes
the preview. F2 restores the launch state paused, including funds, counters,
entities, RNG, pending effects, AI state and the human/base cursor.

## Bootstrap and input ownership

`initialize_native_skirmish` creates explicit display-only kind-29/base-model
markers and player starts. It retains the original passable-but-occupied base
visibility encoding, so units can arrive while construction on a base is rejected.
It validates a candidate scene before replacing the session. Duplicate bases,
missing/zero counters, unavailable active bases, invalid simulation dependencies,
map-only initial state and fewer than two active hostile teams are rejected.
Campaign level changes cannot silently replace a skirmish session.

Tab and held-release controls skip AI/inactive owners. If player zero is AI,
the first human is selected. All-AI configurations remain inspectable as an
observer, with individual build, factory, spawn and release controls disabled.
Global preview controls (pause/tick, camera and development simulation toggles)
remain available. Existing script/replay controls are unchanged.

## Dynamics arrival fix

The exact shipped-config test exposed ordinary units stalling immediately before
entering an enemy base when velocity dynamics were enabled. The transport adapter
copied entity bytes +0C..+0F into steering position W. Those bytes encode index,
radius and kind; they are not a spatial coordinate. Target-cell steering subtracts
all four lanes, and the resulting nonzero W failed the orientation helper's
admission check, repeatedly discarding motion at the cell boundary.

The adapter now supplies XYZ plus zero W, preserving the packed metadata and all
four quaternion lanes. The original numeric leaf implementations are unchanged.
A private temporary diagnostic traced the failed return to orientation admission;
the public shipped-config encounter failed before this fix and reaches base
counter depletion with dynamics, contacts and weapons enabled afterward.

## Validation and limits

- 60 headless native CTests; 62 with SDL/OpenGL, including two graphics tests.
- Keyboard-driven skirmish chooses human owner 1 while owner 0 is AI, builds at
  the human-base cursor, runs both sides to a mission result, and separately uses
  F2 then rebuilds at the restored base cursor with reset funds/counters/clock.
- Native session tests cover validation rollback, visible/passable/occupied bases,
  both sides producing/deploying, match freeze/restart and the exact shipped config.
- 233 Python tests through unittest and pytest; compileall and repository safety.
- Focused ASan/UBSan: skirmish, deployment input, opponent and mission playback;
  host leak detection disabled.
- Local graphics uses synthetic test geometry with llvmpipe. No fresh original
  asset render or original instruction comparison is claimed. Windows compilation,
  packaging and the existing broader cross-build checks run in repository CI.

This is a limited native skirmish loop. Original player craft/aircraft, adaptive
AI, more weapons and targets, authentic economy/HUD/audio and original mission
objectives/campaign progression remain. Capability estimate: **88% toward a first
playable single-player mission**, from 86%; this is the prior capability yardstick,
not measured source coverage or full-port completion.
