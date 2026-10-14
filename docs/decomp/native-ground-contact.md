# Original ground contact motion

Continues `a437a30` / PR #36. This milestone recovers the motion response in
FE5D8 job routine `7510` and the six-plane world containment path in `7A50`.
It integrates them with opt-in friendly ground-unit contacts. It does not recover
all collision preparation, particle callbacks, enemy capture, damage or combat.

## Evidence and recovered behavior

Input ELF SHA-256 remains
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`;
job SHA-256 is
`9aca1695e6409668d9627ce865ad8008119f1c9e42d81a6106369f5a95e1872f`.
`arena_ground_contact.cpp` implements semantic C++ arithmetic with the existing
SPU inverse-root estimate and round-toward-zero arithmetic. It preserves the
original separating-velocity gates, restitution-scaled normal impulse,
accumulated impulse, movable flag, positional correction and energetic-impact
return. That return is not a damage event.

| Input | Original source | Native use |
| --- | --- | --- |
| Primary radius | entity byte `+0E` times .01, context `+4E0` | Peer overlap and world bounds |
| Lateral radius | entity byte `+32` times .01, context `+4E4` | X/Z bounds; floor with flag `0x02000000` |
| Mass | entity float `+58` | Native preparation of peer response share |
| Restitution | entity byte `+7D` times .01 | Normal impulse scale |
| Movable | entity flags `0x4000` | Velocity and position updates |

Containment checks X minimum, X maximum, Z minimum, Z maximum, floor and ceiling
in original order, using the original chained subtraction and contact kernel.
The `7DD0` caller supplies floor 0, ceiling 2 and arena dimensions. The optional
four sphere-point path remains excluded. Normal finite arithmetic and zero lanes
are covered; denormal/extended-exponent SPU hardware parity is not established.

## Live integration and limitations

`contacts 1` in config/replay or viewer **C** enables contacts and dynamics.
Disabling dynamics also disables contacts. Contacts default off; viewer M still
controls movement. The title reports the selected modes.

Before position integration, a native broad phase gathers nearby packed cells,
deduplicates indices and processes matching-team deployed ground kinds 40, 41,
43..47. It reads peer position/velocity but only writes the transported unit.
Native normal and mass-share preparation omits the original random normal jitter
and original neighbor scheduling. Coincident centers use the recovered
index-directed Z nudge: higher peer index gives -.01, lower gives +.01.
Six-plane containment runs after integration and before steering/velocity update.
Atomic both-bank cell/list/cache/count migration and rollback remain intact.

The original contact response is verified independently of this native producer.
Terrain/corner collision, the original gather/RNG path, peer lifecycle and effect
callbacks, hostile capture and special-kind state remain unrecovered. This is
not a complete port of `7E40` or the full simulation update.

The synthetic replay is:

```sh
comet_construction_replay < decomp/examples/ground-contacts.scenario
```

It produces two units, spawns them into the same cell, verifies separation,
then pauses transport and verifies cells and resource/unit/allocation accounting.
Replacing contacts with dynamics alone fails the separation expectation.

## Validation

Private ps3recomp interpreter revision
`a2c20108af45174dbb7d3e2f87f6acaaef09c6b4` executes original instruction bytes.
Seed `75107814` supplies 10,000 randomized contact queries and 5,000 six-plane
queries: all **180,000 float lanes and 15,000 return values match**, including
2,673 energetic responses and 3,248 boundary-hit cases. XYZ inputs vary; W is
zero. The external `1E880` effect callback is intercepted to resume at its return
address, leaving motion instructions untouched but suppressing effects. Original
instructions, assets and private harnesses remain outside tracked source.

Public tests include numeric references, movable/separating behavior, all planes,
corners, floor-radius selection, invalid inputs, both coincident index orders,
no peer mutation, rollback and accounting. Independent review caught the initial
reversed coincident nudge; both index-order regressions failed before correction
and now pass. Review found no remaining actionable issues.

Release builds with warnings as errors pass all **30 native/CLI/graphics CTests**
and **233 Python regressions**. Focused contact/transport ASan and UBSan checks
pass with leak detection disabled because host process inspection is unavailable.
Offscreen viewer input tests exercise M/N/C/T. On original level 0, two live units
run with contacts selected; unit 9 moves `(14.5379,3.4551)` to
`(15.6818,4.56908)`. The viewer renders three frames with 40 original models,
11 textures and 10 complete framebuffer configurations under Mesa llvmpipe.
Windows validation is performed by the native CI workflow on the published PR.

## Remaining work and estimate

Next recover original neighbor gather/normal jitter and terrain/corner collision,
then remaining base/height/special-kind state and enemy capture/weapons/damage.
Automatic deployment, enemy AI, mission win/loss, HUD/control parity, audio/save
and full-game parity remain unfinished.

Estimated capability: **50% toward a playable single-player mission**, up from
48%. This is a judgment of usable capability, not percentage of original code
recovered. No combat or mission-completion credit is added.

Follow-up: [original pair preparation](native-ground-pair-preparation.md)
replaces native normal/distance/mass arithmetic and supplies original random jitter.
