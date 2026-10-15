# Ground health depletion and corpse lifecycle

## Recovered paths

Job FE5D8 `7E40`, branch `83D8..83F0`, clears a kind-45 victim's health
at `+0x38` when it overlaps a hostile ready kind-45 peer. Ownership and
statistics do not transfer in this branch. Readiness now follows the separately recovered
[converter pulse](native-converter-health.md). Signed queued health effects also
feed this lifecycle; firing/projectile producers remain unfinished.

The ground caller invokes `6F40` after motion. Its depleted-health branch
`7368..74F0` reaches `4B68` unless flags `+0x60` contain `0x08000000`.
Notifications and visual-effect calls are excluded from the native leaf.

`4B68..4CBC` unregisters the original owner once and converts to kind 62:

| Field | Transition |
| --- | --- |
| `+0x0F` | kind 62 |
| `+0x33` | clear stage bits, preserve owner and low category bits |
| `+0x80` | duration bits `3ECCCCCC` |
| `+0x84` | age zero |
| `+0x88` | copy original scalar `+0x78` |
| `+0x8C` | `.5` |
| `+0x90..9F` | copy original velocity `+0x40..4F` |
| `+0xA0` | copy original scalar `+0x54` |

`6578` clears common flag bit 1. Its context guard prevents a second
unregistration after `4B68`. With flag `0x08000000` set, death unregisters and
requests direct retirement instead of creating a corpse. Native storage clears
retired records and returns indices FIFO; original kind-63 tombstone construction,
three-tick delay and asynchronous job completion remain outside this adapter.

The runtime dispatcher setup `A5F8..A8C8` assigns kind 62 to `12DC8`.
`12DC8..12EE4` advances position with current velocity and elapsed time,
computes decay from the **old** age/duration, updates velocity from the saved
initial vector, and updates scalars `+0x78/+0x54`. It stores new age and expires
only when **new age > duration**. Reciprocal estimates, refinement, FMA ordering
and round-toward-zero arithmetic are retained. Offset names stay explicit where
the original field's broader meaning remains uncertain.

## Owned simulation and scene

Health depletion runs after each live ground movement substep. Death releases
active quota immediately; an ordinary corpse retains its pool slot until expiry.
Corpses use bounded substeps, update both banks and ordered packed cell records,
and no longer participate in live-ground capture/contact gathering. Whole frame
storage/statistics/RNG changes remain staged in the existing simulation adapter.
The transport preview's pause control also pauses corpse advancement.

Native record replacement distinguishes `Updated`, `CellFull` and `Invalid`.
When a corpse's destination is full, its position stays put while age and decay
continue; expiry can therefore still release capacity. This is an explicit native
capacity policy, not a claim about the original job's spatial-list scheduling.
Malformed banks/membership/numeric data reject without record mutation.

Tick results expose health-depleted and retired indices. Scene cleanup uses the
retired indices before new construction is projected, preserving structure links
and index reuse. The scene retains a kind-62 model until expiry. A native preview
scale uses the ratio of decayed `+0x78` to saved `+0x88`; this shrink treatment is
not a recovered original rendering/FX pipeline. Ordinary entities retain scale 1.

## Reproduction and verification

```
cmake -S decomp -B build-decomp
cmake --build build-decomp
build-decomp/comet_construction_replay < decomp/examples/ground-lifecycle.scenario
ctest --test-dir build-decomp --output-on-failure
```

The scenario produces two opposing kind-45 units, explicitly readies one,
observes health depletion, active-quota release, temporary corpse retention,
expiry, pool release and renewed production. Replay adds `expect corpses N`,
`ground-depleted` and `entity-retired` output.

Private original job SHA-256:
`9aca1695e6409668d9627ce865ad8008119f1c9e42d81a6106369f5a95e1872f`.
Oracle interpreter: `a2c20108af45174dbb7d3e2f87f6acaaef09c6b4`.

- 10,000 original corpse updates and 10,000 original death transitions match all
  **6,400,000 output bytes**, including full entity records and death statistics;
  6,130 corpse-step queries cross expiry.
- Extended hostile-contact comparison: 10,000 queries including kind-45 victims
  match all 7,680,000 output bytes and ownership-change classifications (321
  captures). These classifications do not claim whole-function return parity.
- Existing 10,000 base-arrival and 10,000 resource/counter comparisons remain green.
- Public tests cover live/no-op, malformed numeric and accounting rollback, direct
  retirement, original fields, natural contact-to-corpse-to-expiry, scene removal,
  ordered migration, full destination and quota/pool conservation.
- Independent review found a valid full-cell corpse migration could freeze the
  frame. The owned adapter now preserves age/expiry under that capacity rejection;
  disabling the fallback reproduces the failing regression assertion.

The leaf's native domain uses finite normal/zero floats, positive duration at
most 10 seconds, age 0..10 and elapsed `(0,.25]`. Owned positions are bounded to
64 and cell coordinates clamp to configured extent. Non-IEEE extended SPU
exponents, subnormals and overflow reject; hardware-wide parity is not claimed.
Validation passes: 36 headless CTests, 37 clean graphics-enabled CTests,
233 Python tests, compileall, repository safety, and ASan/UBSan (host leak
inspection disabled). Original level-0 assets render a temporary corpse at
fraction .625 and then verify expiry: 40 models, 11 textures, 10 complete
framebuffers, three rendered frames. This uses explicit native unit placement
and readiness; autonomous mission play is not established.
Original artifacts, generated instruction execution and harnesses remain private.

## Remaining work and estimate

Automatic deployment and capture readiness, weapon targeting/projectiles/damage,
original neighbor scheduling and delayed tombstones, aircraft/special-unit
lifecycle, authentic effects/economy/HUD/audio, enemy AI, and mission win/loss/
restart remain incomplete. No playable-mission or complete-combat claim.

Estimate: **66% toward a playable single-player mission**, up from 64%.
This credits the recovered death/corpse path and reusable capacity, rather than
measuring source coverage or assuming the remaining work is linear.
