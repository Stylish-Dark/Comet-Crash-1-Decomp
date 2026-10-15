# Original converter activation and health accumulation

Continues PR #46 at `06eb1e4`. Automatic converter capture now uses the
original kind-45 pulse, rather than requiring a manual readiness override.
Signed health effects can be queued into the native simulation and consumed
before its existing ground death/corpse lifecycle. This does not implement
weapon firing, projectile production, target selection or enemy decisions.

## Converter pulse

Original job FE5D8 dispatches kinds 40..48 through `18030`. Its call order is
`14060` motion/contact (including `7E40` capture), `158B8` activation/velocity,
then `6F40` lifecycle. Kind 45 reaches `16140..162AC` from `16134`.

The recovered leaf:

- Advances entity `+98` by the original ordered calculation of
  `elapsed * 4 * float_bits(3FC90FDA) / 3`. This is approximately a three-second
  cycle; division preserves the original reciprocal refinement.
- Strictly above `float_bits(40C90FDA)`, subtracts that value once through
  `1801C..18028`. It does not use `fmod` or repeated wrapping.
- Evaluates the original reduced sine/cosine polynomial and sets byte `+BB`
  when the sine is strictly greater than `float_bits(3DCCCCCC)` (approximately .1).
- Produces context scalar `+18 = max(sine * 9, float_bits(3F333333))`.
  The native simulation persists it as the entity's raw core scalar `+18`;
  no visual-effect or speed meaning is assigned to this field.

The native wrapper supports stage-5 kind 45, finite phase in `[0, 2pi]` and
elapsed in `[0, 3]`. Those bounds preserve the single-wrap invariant. It restores
the caller's rounding mode after round-toward-zero arithmetic.

Positive `+8C` takes the separate `16F70..16FFC` branch: decrement the cooldown
without clamping, clear readiness, preserve phase, and multiply old core `+18`
by `float_bits(3F199999)`. Crossing zero still suppresses that update's pulse;
the next update may pulse. These state writes are recovered and tested, but the
original branch additionally scales velocity-integration/speed-limit registers
by `(new core18 + float_bits(3E999999))`. **That motion damping is not integrated.**
Supported native unit constructors initialize `+8C` to zero and no supported
native producer sets it positive. Full stun/cooldown motion remains a later task.

The native simulation applies activation after each converter's contact/motion
attempt and before death, including when movement has no target or is blocked.
After movement stops it receives the remaining bounded activation substeps
without retrying motion. Peer processing retains the existing deterministic,
ordered native bank policy; original SPU read/output-bank scheduling and gather
order are not claimed. The debug `capture-ready` command remains a one-time seed
that subsequent automatic updates replace.

## Signed health effects

The original area-effect producer calls `16F8` with a negative damage value at
`90C8..90D4`. `16F8` resolves a 128-byte accumulator indexed by entity ID;
`17F0` adds the signed incoming delta to its float lane `+10`, stored at `1824`.
Positive deltas heal and negative deltas damage.

Consumer `1AD8` snapshots that lane, clears it and commits the accumulator
through atomic helpers `A9F8/A9B0`, then returns the saved value at `1F80`.
Caller `9EC4` consumes it. The `9ED0..9EDC` branch suppresses health writes
under entity flags `+60 & 0x00100000`. Otherwise `A1D8..A210` writes:

`health(+38) = min(maximum(+64), health(+38) + delta)`

There is no lower clamp. Suppression still consumes the accumulated effect.
The portable helpers preserve other bytes and caller rounding. Nonfinite and
subnormal operands are rejected; accumulator overflow rejection is native policy.
Comparison coverage is the finite normal/zero domain, not SPU extended-range
or hardware arithmetic parity.

`queue_ground_health_delta(index, delta)` accepts only a live supported ground
unit, merges effects in order and stages them in owned storage. The next
positive advance with ground transport enabled applies them once before
movement and death. Motion-off previews retain pending effects. Failed advances
retain them; suppressed effects are consumed. Retirement/reuse cannot carry a
previous unit's effect into a new identity. Scene/playback checkpoints copy the
pending vector along with all other state, so F2 restores pending effects too.
Only this health lane is implemented; original accumulated impulses, source
attribution, area falloff/filters and atomic contention are outside the boundary.

## Repeatable demonstration

Run `decomp/examples/automatic-capture-health.scenario` through
`comet_construction_replay`. It produces a converter, lets its original pulse
activate, introduces a scout, verifies automatic capture, applies combined
damage/healing and a capped heal, then verifies a lethal effect, quota release,
temporary corpse and final index retirement. Resources/tuning/placements and
explicit `health-delta` input are development fixtures, not original weapon AI.

New replay commands are `health-delta INDEX AMOUNT`,
`expect-unit-health INDEX VALUE` and `expect-capture-ready INDEX 0|1`.

## Validation

The canonical user-owned ELF SHA-256 was checked against
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
Private harnesses execute untouched original instructions with successful
synchronous accumulator transport where required:

| Boundary | Fixtures | Exact comparison |
| --- | ---: | --- |
| Converter pulse, including thresholds and wrap | 20,000 | All 256 entity + 32 context bytes |
| Converter cooldown state writes | 10,000 | All 256 entity + 32 context bytes |
| Health application/suppression | 10,000 | All 256 entity bytes; 5,005 applied / 4,995 suppressed |
| Signed accumulation | 10,000 | Float word, including signed-zero and rounding cases |

Public tests also cover malformed values, byte preservation, rounding restoration,
multiple effects, one-time consumption, rejected clock retention, corpse expiry,
recycled slots, restart restoration and no-target converter timing. Independent
review caught the no-target timing truncation; the regression failed before the
fix. All 233 Python tests and 43 headless / 44 graphics native CTests pass;
focused ASan/UBSan checks pass with host leak detection disabled.

A private original-level-0 integration run verifies automatic capture, scout
health 35 after merged effects, converter death/corpse expiry, synchronized banks,
and three rendered frames with 40 original models, 11 render textures and ten
complete framebuffers. Original binaries/assets, disassembly and oracle harnesses
remain private. No weapon firing or autonomous battle-completion claim is made.
