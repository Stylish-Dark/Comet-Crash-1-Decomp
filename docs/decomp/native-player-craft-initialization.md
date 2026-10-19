# Original player-craft body initialization

Continues PR #56 (`c5860ba`). Both regression and native-source-port workflows
passed, including Linux graphics and Windows build/package jobs.

The F3 pilot's collision shape now comes from the recovered original type-1
simulation-body initializer. Its primary and lateral radius are **.24**, replacing
the previous native half-cell guess. Restitution 100 and the movable flag are now
source-backed as well. A reusable packed-body initializer supplies the next
craft entity/contact/carrying recovery; this milestone does not register the
preview ship as a combat entity.

## Identity and provenance

ELF SHA-256 remains
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
FE5D8 job SHA-256 remains
`9aca1695e6409668d9627ce865ad8008119f1c9e42d81a6106369f5a95e1872f`.

SPU `A610..A674` installs `12B88` at dispatch-table `26980+4`, proving that
this craft update belongs to type **1**. The PPU creation path passes type 1 to
`FECFC` at `CFADC`, registers it through `CA514` at `CFB8C`, then stores its index
in player `+220` at `CFBC8`. This identifies the simulation body independently
of visual model names. Its stored model byte is 40; the F3 preview still renders
the original slot-0 ship model under the existing native rendering policy.

`initialize_arena_player_craft_entity` replaces the bounded slot writes beginning
at `CFAB8`, including the shared `FECFC` initializer, and ending at `CFB88`.
Allocation, the preceding grid/position preparation, registration and subsequent
player/index/resource links are outside this function. The source spawn Y is
zero; X/Z are explicit finite inputs and a caller-supplied staging W is copied
to `+EC`. Core `+0C/+0D` and velocity W retain reused-slot bytes as in `FECFC`;
quantity/type overwrite `+0E/+0F`. These packed words are not spatial W lanes.

| Field | Source value |
| --- | --- |
| Type / stored model / stage | 1 / 40 / 6 |
| Primary and lateral radius bytes `+0E/+32` | 24 / 24 |
| Constructor flags / final flags | `0000C000` / `0000C006` |
| Restitution byte `+7D` | 100, overriding shared default 150 |
| `+58` | 70 |
| `+3C` | 100 |
| `+38/+64` | 1000 / 1000 |
| `+10/+78/+5C` | 64 / 1 / 2 |
| Vectors `+90/+A0` | `(0,1,0,0)` |
| Quaternion `+F0` | `(0,0,0,1)` |
| Vector `+E0` | source spawn `(X,0,Z,stagingW)` |
| Halfwords `+B8/+C4/+C8/+CA/+CC/+CE` | `FFFF` |
| Bytes `+BA/+BB/+BC` | 1 |
| Bytes `+BD/+BE/+BF` | 0 |
| Byte `+C6` | owner |
| Words `+74/+B0/+C0/+D8/+DC` | zero |
| Float `+D4` | 2 |

The selected caller constants are loaded at `CE414`, `CE764`, `CEA20`, `CEA28`
and `CF180`; `CFAA0` supplies mass 70. Vector constants resolve through TOC
`241DB8` to `1BCD60` and `1BCEB0`. Unwritten bytes are preserved.

## Live integration and boundary

`arena_player_craft_bounds` initializes a local original body and reads its
radius/material flags. It reproduces `9B90/9C60` radius staging with
round-toward-zero and the original multiplier bits `3C23D70A`.
`24 * multiplier` is exactly float bits `3E75C28F` (~.24). The caller's rounding
mode is restored. Native supported extents are 1..24; owners 0..3 and finite
spawn coordinates/staging W of magnitude at most 1,000,000 are validated before
any slot mutation.

NativePilot caches those bounds on construction/reset. It retains the prior
flight/contain/response sequence and valid build-cursor cells, including a
one-cell arena. Source mode selection, initial pilot height .85, visual heading,
pause/reset/cancel policy and rendering/control ownership remain native choices.
The public generic `ArenaCraftBounds` .5 defaults remain explicit generic preview
defaults; only the pilot switches to the constructor-derived profile.

## Validation

A private fail-closed evaluator decodes the untouched original PPU instructions
with Capstone. It executes the encountered integer/memory/float/vector-selection
instructions in `CFA80..CFB88` and `FECFC..FEE10`; original instruction bytes are
not substituted. Registers and stack pointers at the entry boundary are staged
explicitly from the source caller, with initial Y=0. Unexpected instructions or
step-budget exhaustion abort the comparison. This bounded evaluator is not a
hardware-perfect PPU emulator and does not validate allocation or player links.

Seed **57019**, **1,000** synthetic cases, all four owners, randomized X/Z and
staging W, randomized reused-slot bytes: **all 256,000 output bytes match**.
Concatenated-output SHA-256:
`dc342eca7a66bebbbc6b328d9fe2bd8784e93b9ca2262ebf0dda41790b83dba0`.
Sixteen public fixtures contain only synthetic inputs and full-slot FNV-1a
fingerprints. Original bytes, disassembly and private evaluator remain outside
Git. This comparison adds initializer evidence; it does not repeat or broaden
PR #56's SPU motion-parity claim.

The pilot radius assertion failed on the previous .5 profile before the change,
then passed with the recovered profile. Public tests cover source fields,
preserved bytes, invalid-input atomicity, full-slot fixtures, rectangular bounds,
rounding restoration, repeated diagonal containment and escape from walls.
A hand-written test initially expected the adjacent lower radius bit pattern;
checking the exact source product corrected that expectation before completion.

Validation completed: **64 headless / 67 graphics-enabled CTests**, **233 Python
regressions**, **five focused ASan/UBSan CTests**, compileall, repository safety
and diff checks pass. Sanitizer leak detection is disabled for this host.
Independent review found no actionable issues and reran the two focused native
tests. SDL graphics use synthetic geometry and a virtual controller; physical
controller and fresh original-asset rendering are not claimed. No graphics
rerun was needed in this milestone.

## Remaining work and estimate

Next recover the craft `3238` world query / terrain and entity contacts, then the
`+51 & 1C` gate, attachment/action and angular paths. Registering the actual craft
body and recovering its player/economy/carrying/damage links remain necessary;
this initializer alone does not supply them. Aircraft, original adaptive AI,
mission/campaign rules, HUD/audio/save and presentation parity also remain open.

Estimate stays **92% toward the first playable single-player mission**. Removing
an unproven physical parameter and recovering the packed initializer improves
source fidelity and prepares entity integration; it does not complete a new
mission interaction. This estimate is not source coverage or full-port progress.
