# Native gamepad controls and recovered stick normalization

The interactive construction/skirmish viewer accepts the first SDL-recognized
controller. Connect before launch or hotplug it; keyboard controls remain usable.
The overlay switches to controller hints while a controller is attached.

| SDL controller control | Preview action |
| --- | --- |
| Left stick / D-pad | Move build cursor |
| A | Submit selected structure |
| LB / RB | Previous / next structure type |
| X or right trigger | Hold ground-unit release |
| B | Cancel held input; release all controls to rearm |
| Y | Cycle factory mode at cursor |
| Left stick click | Next ground-unit kind |
| Right stick click | Next human owner |
| Start | Play/pause |
| Back | Restart launch state, paused |

Mappings are SDL's standardized labels; physical labels vary by device.
Only one controller is active. Removing it pauses the mission, cancels both
keyboard and controller release and selects another available controller.
Focus loss also pauses/cancels. Connect, cancel, pause, owner change and restart
require neutral controller axes and released mapped buttons before rearming.
Independent keyboard/controller holds keep one source's release from canceling
the other. Existing human/observer and finished-match mutation guards apply.

## Original evidence

The pinned original ELF has SHA-256
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
The pad decoder begins at PPU `0x000F1500` (OPD `0x236FC0`, TOC `0x241DB8`).
Its four scalar axis sequences at `0xF1728..0xF1868` normalize an unsigned byte:

```
x = (raw - 128) * 0.0078125f
if abs(x) < 0.2f: return +0
return (x + (x < 0 ? 0.2f : -0.2f)) * 1.25f
```

The implementation preserves single-precision stages, strict comparison,
nearest rounding and the asymmetric positive endpoint (`255 -> 0.990234375`).
It restores the caller's rounding mode. State outputs are left X/Y at +A8/+AC
and right X/Y at +B0/+B4; this adapter uses only the left stick.
A private interpreter of the untouched first scalar sequence `F1728..F1778`,
reading constants from the ELF, produced all 256 expected float bit patterns.
The public test checks every input using FNV-1a over 32-bit words:
`5c60b3ebccbac445` (offset basis 14695981039346656037).
SHA-256 of those words serialized big endian:
`96ed88e8b6d1bc9f879f6f51c5889e856b4b2e82744d5839014e7cdf807f503c`.
This is an instruction-subset oracle, not a full pad decoder or PS3 runtime test.
Original executable bytes and private tooling are not included in Git.

SDL signed axes are rounded to the nearest unsigned-byte position before this
leaf. Sign-to-cell conversion, initial 0.2-second/repeat 0.08-second navigation,
button mappings and the right-trigger threshold (>3276/32767) are native preview
policy. They do not recover craft physics, original action bindings, pressure
buttons, full original repeat logic or multiplayer controller assignment.

## Validation

The CPU test covers all axis bytes, rounding restoration, deadzone, navigation,
button edges, holds, neutral rearming and invalid elapsed-time rollback.
The SDL/OpenGL test attaches a virtual controller and uses real button/axis polling
to navigate, build, produce and release units. It exercises focus loss, hot-unplug
and restart while release is held; the second run also catches SDL hint lifetime.
Geometry is synthetic and rendering uses offscreen llvmpipe locally. A physical
controller and original-asset rendering have not been tested for this milestone.

The first-playable-mission capability estimate is **89%**, up from 88%.
This is a judgment of the existing milestone checklist, not measured source
coverage or full-game completion. Original craft/aircraft, richer AI, further
weapons/targets, authentic economy/HUD/audio and campaign objectives remain.
