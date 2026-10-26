# Original full-action particles and hard craft containment

Checkpoint2026-10-26, above PR #68. P1/P3/P5/P6 advance within Partial;
G1/G2 remain unaccepted. Overall complete faithful PC-port estimate stays
about25%, plausible15–35%; no measured code-coverage percentage is implied.

## Recovered source and connected behavior

`arena_particle_emission` recovers FE5D8 job1E880/1E480 with native synchronous
128-byte record publication into the existing original request-pool representation.
It retains2AD8 FIFO allocation, batches at most32, reserve128, head wrap and all
four availability lanes. Partial exhaustion retains earlier batches and consumes
no random numbers for the failed batch. Zero count performs no pool/RNG work.
Native synchronous writes replace DMA; queue bootstrap/ownership remains supplied.

Spherical particles use recovered1F030 direction arithmetic, radius and lifetime
interpolation, then model selection. Directed-circle particles recover1F2F8's
perpendicular bases, near-Y/zero fallback, one random angle and original trig
polynomials. Each spherical/circle record consumes5/3 shuffled-LCG draws.
Thirty-two caller-owned scratch records retain unassigned bytes across batches
and calls.1E480/1EB68..1EBC8 retain position/identity copies, velocity, fade limit,
acceleration, lifetimes and packed flags. Finite packed W subnormal bits remain
outside spatial dots; conservative arithmetic limits remain explicit.

10290 now executes fully expanded11A80..11CA4: four ordered arm particle calls
after proxy publication and before owner emission. Each call first consumes its
original model85..88 selection draw, even if the particle pool is full. Missing
particle transport rejects before pending, visual, proxy or RNG mutation.
Simulation/scene APIs forward the transport; full attachment reaches6578 and
both authoritative banks with actual pool-exhaustion behavior.

Hard containment was previously blocked whenever7510 reported an energetic hit.
10500/10534/1053C clears5A0 before10548 ->7DD0.7638..76F0 therefore computes
particle count0 for this controller, including when74 was positive at entry.
1E880 skips allocation/RNG and only synchronizes DMA. Native synchronous storage
needs no publication for that call: original hard-wall/corner physics and bank
commits now complete. Real-peer contacts and other7510 callers remain separate.

## Evidence and limits

Private original-instruction comparison executed1E880/1E480/1F030/1F2F8, capturing
allocation/publication at existing boundaries and supplying the recovered204E0
shuffled-LCG stream.24 emitter cases matched all128-byte records, pool state and
RNG continuation across both shapes, dirty scratch, reuse, wrap and exhaustion.
15/16 full-action cases matched complete0x680 contexts, proxy/particle records
and RNG; one inherited near-axis numeric exclusion remains.16 hard-wall/corner
cases matched full contexts and zero particle/RNG work. Independent source review
confirmed publication order/exhaustion and the zero-count containment finding.
These comparisons are scoped, not hardware concurrency or compiler-byte matching.

A workspace reset interrupted publication. Source was restored from this session;
public fixtures and build checks were rerun after restoration. Release build and
all74 CTests pass; two zero-byte executables required relinking and only their
failed launches were rerun. Safety/whitespace checks pass. Private interpreter
inputs/tools must be restored before extending the original-output comparisons.

Public fixtures pin an original128-byte output fingerprint/RNG continuation,
reserve/partial-batch behavior, circle draw count and packed W. A registered-scene
scenario rejects missing transport transactionally, then commits full attachment
with three of four particles published before exhaustion. A hard-corner fixture
then commits without particle transport. Attachment and unattached links are
explicit original-state fixtures; no pickup/drop mutation is inferred.

Next: actual pickup/drop links and carried-ground updates, expired attachment,
12288 death, real-peer contacts, pending transport, original scheduling and
command/mouse integration. Particle rendering/downstream consumption remains
unfinished. Existing preview controls do not establish the original player loop.
