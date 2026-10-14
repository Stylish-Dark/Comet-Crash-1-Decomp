# Original ground pair preparation

Continues `9974004` / PR #37. This milestone replaces the live contact adapter's
host square root/division and unperturbed normal with recovered pair arithmetic.
It also recovers the complete `1F030..1F2F0` random-direction leaf used at `806C`.
Original neighbor gathering, peer scheduling and terrain collision remain separate.

## Recovered arithmetic

`arena_ground_pair.cpp` is semantic C++, with round-toward-zero scopes, preserved
FMA order and shared numeric reciprocal/inverse-root estimate buckets. It does
not execute original instruction code at runtime.

| Window | Recovered behavior |
| --- | --- |
| `1F030..1F2F0` | Two shuffled-LCG draws, angle conversion, quadrant reduction, sine/cosine polynomials and vector assembly |
| `7FB0..803C`, `81C8..824C` | XYZ difference, refined distance, near-coincident index-directed Z correction |
| `8044..811C` | Refined reciprocal normalization, random vector scaled by original ~.1 constant, final inverse-root normalization and overlap gap |
| `8320..8368` | Peer mass divided by total mass with original reciprocal/refinement; zero share when total does not exceed epsilon |

The random vector assembly is `(cos(a)*cos(b), sin(b), sin(a)*cos(b))`, retaining
the original polynomial and vector multiplication/FMA order, including zero signs.
It uses the existing per-simulation shuffled LCG, not a new random generator.
Each eligible native-gather peer consumes two draws even when it does not overlap.
The response share need not equal host division bit-for-bit: equal positive masses
produce `0x3EFFFFFF`, rather than exactly .5. The overlap gap uses an FMA for peer
radius subtraction, followed by a separate own-radius subtraction.

`prepare_arena_ground_pair` returns corrected position, normal, gap, share and an
overlap decision. The adapter passes those to the previously recovered motion
response. Contacts remain opt-in through `contacts 1` or viewer C. Native gathering
still selects nearby deployed friendly ground units in packed-cell order; it does
not claim original eligibility, gather order, enemy capture or peer callbacks.

## Transaction and numeric boundaries

The live adapter only commits its local slot and copied RNG after route checks and
both-bank migration succeed. Invalid peer mass, blocked transport and other failed
transactions leave banks, cells and visible RNG unchanged. Peer records are read,
not mutated. The packed W lane contains entity metadata, so live pair inputs use
XYZ with W zero and writes preserve the header bytes.

Normal finite arithmetic is the validation domain. Exact-zero center distance uses
an explicit zero-distance guard before the existing index-directed .01 Z fallback;
this avoids the private interpreter's unsupported extended-exponent zero estimate.
Exact-zero hardware parity is not asserted. Nonzero near-coincident original paths
are compared directly. Denormals and SPU extended-exponent arithmetic remain outside
established parity. The API rejects nonfinite vectors/masses, negative masses/radius,
identical indices and an invalid jitter vector. Mass validation also applies to
non-overlapping gathered peers. Non-overlapping peer velocity is not consumed by
the response, so it does not trigger that response's finite-velocity validation.

## Validation

The original ELF hash remains
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`;
job hash remains
`9aca1695e6409668d9627ce865ad8008119f1c9e42d81a6106369f5a95e1872f`.
A private ps3recomp interpreter at revision
`a2c20108af45174dbb7d3e2f87f6acaaef09c6b4` executes unchanged original arithmetic
instruction bytes. Seed `80448368` drives:

- 10,000 complete random-direction cases with random 32-bit generator seeds;
- 15,000 pair-preparation cases, including 1,500 nonzero near-coincident cases,
  both index orders, 6,332 overlaps and zero-total-mass cases;
- 190,000 matching float lanes and 25,000 matching next-RNG values.

The pair harness supplies W-zero inputs to registers at the `7FB0` arithmetic
boundary, then executes the original windows, including the original random leaf.
Mass-share output is evaluated separately at `8320..8368` for each query. This
validates the exposed arithmetic interface, not the entire packed-record caller or
full `7E40` routine. No raw instructions, original assets or private harnesses are
tracked. Public tests contain numeric outputs and behavioral regressions.

The reference tests failed with empty implementations. The original direction
references caught an initial axis-order error, which was traced and corrected.
The transport RNG-consumption regression failed before integration and now passes.
Independent code review found no actionable issue and checked nested rounding
restoration, transactional state and extreme mass samples against the oracle.

Release warnings-as-errors builds pass all **31 native/CLI/graphics CTests** and
**233 Python regressions**. The first Python run lacked the reset environment's
Capstone dependency; after restoring it the complete suite passes. Focused pair
and transport ASan/UBSan checks pass with leak inspection disabled because this
host does not allow process inspection. The original two-unit level-0 scene moves
unit 9 from `(14.5379,3.4551)` to `(15.6851,4.56667)` and renders three frames:
40 models, 11 textures and 10 complete framebuffer configurations under llvmpipe.
Windows native execution is validated by CI on the published PR.

## Remaining and estimate

Next recover original neighbor gathering/scheduling and terrain/corner response,
then remaining base/height/special-kind state and hostile capture/weapons/damage.
Automatic deployment, enemy AI, mission win/loss, HUD/control parity, audio/save
and full-game parity remain unfinished.

Estimated capability: **52% toward a playable single-player mission**, up from
50%. This credits recovered pair preparation and original randomized live contact
normals, not combat or mission completion. The estimate measures capability, not
percentage of original source recovered.

Follow-up: [original terrain walls and corners](native-ground-terrain.md)
adds local obstruction response to live contact mode.
