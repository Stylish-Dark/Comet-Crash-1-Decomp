# Interactive native ground deployment

The recovered selected base-release path is now usable from the native viewer.
This is native input and status presentation, built on PR #44's original timing,
claim, constructor and allocation recovery. No new original controller or combat
routine is claimed recovered by this milestone.

## Try it

Build with `COMET_NATIVE_VIEWER=ON`, SDL2 and OpenGL, then run:

```sh
build/native/comet_native /path/to/maps 0 --assets /path/to/extracted/data --arena --construction-config decomp/examples/interactive-ground-level0.cfg
```

Use the directory containing extracted `level0.map` for `/path/to/maps`.
The configuration enables factory production, transport, recovered velocity
and contacts. It supplies synthetic tuning, funds, unit capacity and impact
budgets; these are development inputs, not authentic level-0 balance.

The viewer starts paused. Press P to advance factories and movement, and hold D
to release available orders of the selected kind from the player's fixed base.
Existing imported factories can produce orders; use 7/B to build another factory
at a free highlighted cell. Arrows move that cell. F5..F9 select a hovered
factory's production mode. Its mode/timer remain in the window title.

| Control | Result |
| --- | --- |
| Q / E | Previous / next ground kind: 40, 41, 43, 44, 45, 46, 47 |
| Hold D | Full-strength release through the existing selected-kind timer |
| Release D | Stop release, preserving elapsed timer |
| Tab | Stop the previous player's hold, then switch player |
| P | Toggle playback; pausing stops held release |
| T | Advance one second while paused; held release also works with ticks |
| F2 | Restore viewer launch checkpoint paused |

Selection persists separately for each player. Aircraft 42/48 are omitted.
Changing type while held updates the selected release type. Switching player
requires a fresh D press; an existing hold is never transferred. Focus loss
pauses playback, clears queued manual ticks and cancels held release; focus
regain does not resume. Viewer exit/error unwinding also clears its held input.
Configured scripted release remains intact until that owner is manually
controlled, so existing automated previews keep working.

## Status panel

A small native bitmap alphabet displays player, funds, active/pending counts,
selected kind's remaining orders, HELD/SCRIPTED/IDLE, playback state, configured
base counters and key hints. An unset counter is `--`. Complete active base tables drive native preview
victory/defeat/draw. Finished matches freeze; F2 restarts. See
[native mission lifecycle](native-mission-outcome.md) for limits.
The panel wraps at narrow window widths. It uses a window-sized viewport after
arena presentation and restores OpenGL state, so arena supersampling and
wireframe state do not hide or distort the text. No original font assets are
needed. This is development feedback, not the original game HUD.

`NativeDeploymentInput` is independent of SDL and owns input lifetime only.
Simulation release timing, success reset, order consumption and failure semantics
remain unchanged. No release occurs merely because an input key event arrives;
normal simulation advancement performs it.

## Validation and boundaries

Test-first checks cover selected-order consumption, key-up, per-player kind
restoration, wraparound, repeated press, cancellation, scripted input coexistence
and exit cleanup. SDL-injected tests exercise Q/E, D down/up, owner switching,
focus loss, pause and exit against a real scene, then verify projected units and
quotas. A captured-frame pixel assertion checks visible panel text.

Final local validation: 38 headless / 39 graphics CTests, 233 Python tests,
compileall, repository safety and ASan/UBSan (host leak inspection disabled).
Original assets validate level-0 startup with 40 models, 11 textures and 10
complete framebuffers. A private real-asset test produces and releases a unit
through this input class, verifies next-tick motion, and renders the panel.
Independent review found no blocker; its timing-scope concern was resolved by
preserving existing elapsed-time behavior, and pause coverage was added.

Original moving craft/gamepad controls, automatic capture activation,
weapon targeting/projectiles/damage, aircraft, autonomous enemy behavior,
authentic economy/HUD/audio/effects and original mission/campaign rules remain open.
Native preview freeze/restart is now implemented separately.
Estimate: **71% toward a playable single-player mission**, up from 69%; a
capability judgment, not source coverage. This increase credits usable control
and feedback for the production/deployment loop, with no combat recovery credit.
