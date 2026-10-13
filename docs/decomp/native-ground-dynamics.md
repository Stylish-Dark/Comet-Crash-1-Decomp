# Original ground velocity dynamics

Continues `e959110` / PR #35. The native velocity update now recovers two bounded
slices of FE5D8's original simulation job: acceleration at `15998..159DC`, and
velocity integration/horizontal limiting at `15C38..15C80` plus
`162B0..162FC`. This is not a complete decompilation of `158B8`.

## Evidence and semantic names

The exact input ELF was revalidated against SHA-256
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
The raw job hash remains `9aca1695e6409668d9627ce865ad8008119f1c9e42d81a6106369f5a95e1872f`.

`9A70` copies entity position and velocity to context `+480/+4A0`.
`A058..A06C` sets context `+5D0` to the entity subobject at `+80`.
This proves the tuning pointer used by `158B8` points into the same unit record:

| Field | Consumer | Meaning |
| --- | --- | --- |
| entity `+80` / tuning `+00` | `159A4/159B8/159C0` | steering acceleration magnitude |
| entity `+84` / tuning `+04` | `159F8/15C38/162CC` | horizontal speed limit |
| context `+5C0` | `159AC/159C0` | prepared steering direction |
| context `+4C0` | `159D4/159D8` | external acceleration vector |
| context `+520` | `159BC..159D8` | context mask enabling external acceleration at bit `0x20` |

The context mask is derived from entity stage bits by `9C78/9C98/9D44`.
The external vector's world/crowd producer remains unrecovered.

Acceleration is `fma(steering, entity[+80], velocity * float_bits(BC23D70A))`.
The drag constant is approximately -0.01, with its original bits preserved.
Context bit `0x20` adds the external vector, except for kind 47.
Velocity then becomes `fma(acceleration, elapsed, previous_velocity)`.

The horizontal X/Z squared speed is compared strictly with the squared limit.
Only exceeding it invokes the original reciprocal-square-root estimate and
refinement, normalizes X/Z, and reapplies the limit. Y is preserved by the
perpendicular vector mask. The two multiplication masks preserve signed zeros,
including the W lane; they are not bitwise masks. The returned squared speed
is the value before limiting, matching the original live register.

`arena_spu_math.hpp` shares the existing verified inverse-root estimate buckets
with steering. Arithmetic remains semantic C++, not register-machine execution.
Normal finite arithmetic and zero lanes are covered; denormals, SPU extended
exponents and hardware-perfect arithmetic are not established.

## Live integration

| Interface | Action |
| --- | --- |
| Config/replay `dynamics 1` | Select recovered velocity integration |
| Config/replay `dynamics 0` | Select previous immediate velocity policy |
| Viewer N | Toggle dynamics; title displays state |
| Viewer M / replay `transport 1` | Enable movement independently |

Dynamics defaults off. With it selected, target binding cannot overwrite the
accumulated velocity. Transport first advances X/Z using the previous velocity,
then steers at the advanced position and computes velocity for the following
step. This follows the position-before-velocity order of `14060` then `158B8`
in caller `18030`. Fixed altitude, immediate route heading, conservative route
obstruction, boundary clamping and synchronous bank migration remain native
preview policies. World/crowd acceleration is explicitly supplied as zero.
The API can accept a separately recovered external vector later.

The adapter supports deployed ground kinds 40, 41 and 43..47. Whole-kind
special behavior is not established merely by running this common arithmetic
slice; aircraft 42/48 remain excluded. Original special state branches,
orientation interpolation and base interaction are still missing. In particular,
this mode does not claim the prior immediate-velocity policy's settled base
arrival behavior: inertia requires the remaining original base/collision logic.

Run the deterministic synthetic scenario with:

```sh
comet_construction_replay < decomp/examples/ground-dynamics.scenario
```

It verifies selected dynamics, movement, pause/resume, both-bank cell membership
and preserved unit/resource/allocation accounting. A turn regression proves old
velocity advances position before acceleration changes direction. Blocked,
full-cell and invalid dynamics calls retain storage and RNG state.

## Validation

A private ps3recomp interpreter at revision
`a2c20108af45174dbb7d3e2f87f6acaaef09c6b4` executed the untouched windows under
round-toward-zero. Seed `1599816256` generated 10,000 queries varying all four
velocity/direction/external-vector lanes, tuning, elapsed, context flags and
seven supported kinds. All 90,000 acceleration/velocity/pre-limit squared-speed
float bit patterns matched. 3,676 queries took the cap branch; 5,642 included
external acceleration. Original instructions, assets and private harness stay
outside tracked source. Portable tests contain original numeric outputs only.

Release builds use warnings as errors. All 29 native/CLI/graphics CTests and
233 Python regressions pass. Focused dynamics/transport ASan and UBSan checks
pass with leak detection disabled because this host disallows its process
inspection. Offscreen viewer tests exercise M/N/T input and scene projection.
An original level-0 unit moved `(14.4615,3.40072)` to `(15.5929,4.53209)` with
dynamics selected; all 40 models rendered and all 11 textures/10 framebuffer
configurations initialized under Mesa llvmpipe. Windows execution awaits CI.

## Next boundary and estimate

Recover `7E40` collision/crowd preparation and the surrounding `158B8` stage,
height/base and special-kind branches, then weapons/damage and mission rules.
Automatic factory deployment, enemy AI, HUD/control parity, audio/save and
full-game parity remain unfinished.

Capability estimate: **48% toward a playable single-player mission**, up from
46%. Two points credit independently verified acceleration/drag/speed limiting
and its selectable live integration. This remains an incomplete source port.
