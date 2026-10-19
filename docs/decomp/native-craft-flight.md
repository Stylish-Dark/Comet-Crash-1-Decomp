# Recovered craft free-flight step

The F3 pilot preview now uses the original selected free-flight position/velocity
step instead of the prior six-unit fixed-speed cursor. Input gradually changes
velocity in the selected mode, release decelerates it, and position advances with
the previous velocity before the new command is applied. This includes the
original one-update delay when starting or changing direction.

## Proven source path

Pinned ELF SHA-256:
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
FE5D8 raw job SHA-256:
`9aca1695e6409668d9627ce865ad8008119f1c9e42d81a6106369f5a95e1872f`.

PPU `DAD04..DAD78` copies each player's first 0x38 bytes, including +10/+14
movement commands, into root +2F50 with 0x40 stride. The job snapshot begins at
root +A00; player commands therefore appear at snapshot +2550 plus owner*0x40.
FE5D8 `103D0..104E0` gathers these into `(X,0,Z,0)` and scales the target velocity.
This connects the PPU F2350 input cap to a concrete SPU consumer, rather than
inferring movement from model names.

The SPU wrapper `12B88` calls `9A70`, `10290` and `6578`; the recovered slice is
inside `10290`. `9A70` stages persisted position/velocity at context +480/+4A0.
Only these selected arithmetic blocks are replaced:

| Original block | Recovered behavior |
| --- | --- |
| `10438..104E0` | Snapshot +28D4 nonzero selects speed10; zero selects40 |
| `10500..10544` | `position = fma(old_velocity, elapsed, position)`, all4 lanes |
| `105B0..105F8` | Zero-mode-flag response1; subtract/fused response; newY velocity0 |
| `113A8..113DC` | Nonzero-mode-flag response = elapsed * bits40933333 (~4.6) |
| `11DB0..11DC0` | If controller byte+3E is nonzero, target *= bits3F333333 (~.7) |

For each lane, `new_velocity = fma(target - old_velocity, response, old_velocity)`.
The subtraction/multiplication stages must remain separate single operations;
assigning the target directly loses the original rounding behavior. Position
integration occurs first. NewY is zeroed only afterward. The original role of
the mode flag is not named: +28D4 maps to PPU root +32D4. It is not claimed to be
a recovered boost, difficulty or normal-game selector.

## Numeric and integration boundary

The native leaf sets round-toward-zero and restores the caller's rounding mode.
It flushes subnormal operands/results to positive zero, matching the project's
SPU-native policy; denormal behavior is not part of the interpreter comparison.
Finite per-axis commands -1..1, elapsed0...25 seconds, and position/velocity
components of magnitude at most1,000,000 are supported. Invalid/overflowing
queries leave all state unchanged. Caller supplies the already-capped command.

The pilot adapter selects mode-flag1 and reduced-speed0 explicitly. This is a
native choice among recovered branches, pending original mode/state recovery.
It retains the recovered velocity between frames. Preview pause/cancel/focus loss
and disconnect zero that velocity to prevent unintended drift on resumption;
input release while playing decelerates through the recovered step. Restart and
human-owner changes reset both position and velocity. Grid clipping, fixed initial
height, visual heading and control/render ownership remain native preview policy.

The whole `10290` controller is NOT recovered. Its `3238`/`7DD0` world queries,
flag gates, attachment branches, angular motion, collision response, resource
carrying, damage and other action paths remain open. The ship is still a preview
control/render object; ground deployment remains at the base. No full-physics,
original-mission or hardware-parity claim is made.

## Validation

A private harness enters the untouched job at104E0,10500 and105B0/113A8,
returning before unrelated calls. Original instructions are not patched.
Pinned ps3recomp interpreter: `a2c20108af45174dbb7d3e2f87f6acaaef09c6b4`.
10,000 deterministic cases (seed71955) compare all8 float words exactly across
both mode flags and reduced-speed values. Inputs vary all4 position/velocity
lanes, signed commands and delta. SHA-256 of ordered big-endian output words:
`14f1c988338819b370b32bda410450c9f2360593650f69f0cc3ca361500f835f`.
A harness staging error initially omitted the delta load at10504; entering at
10500 corrected the harness before any comparison claim. Sixteen resulting
numeric fixtures are public; original bytes/disassembly/harness remain private.

Public tests cover the fixtures, first-frame delay, release damping, speed modes,
reduced speed, all4 integration lanes, invalid/overflow rollback, rounding restore,
native subnormal policy, pilot stop/reset and all65,536 decoded stick pairs.
SDL tests retain movement-to-build, paused-input suppression and restart coverage.
Geometry remains synthetic/offscreen locally; physical controller and fresh
original-asset rendering are not claimed.

Estimate: **91% toward a first playable single-player mission**, from89%, using
the existing capability yardstick. This is not source coverage/full-game parity.
Next is original collision/attachment/action gating around the now-verified step,
then craft combat/economy integration and aircraft. Campaign/HUD/audio remain open.


Follow-up: [original craft containment](native-craft-containment.md) recovers the
10548 -> 7DD0 boundary call and replaces NativePilot clipping with the original
motion response between integration and velocity response. The exclusions above
describe this milestone's original scope; the whole controller remains open.
