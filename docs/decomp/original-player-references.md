# Original player reference fetch and attached positioning

Checkpoint 2026-10-23, branch `work/original-player-reference-fetch-20261023`,
stacked above PR #66. Completion-map P1/P6 advance within Partial; P2 remains
Partial. G1 and G2 are unaccepted.

## Connected behavior

Recovered FE5D8 B500 ->3108 ->21F8 now fetches actual linked records before the
player preparation/controller path. The simulation/scene staging overload reads
the registered body's current bank, with caller-owned original accumulator
records and job tag. Source startup510..58C makes snapshot+27D0 and source26B90
the same read-bank pointer. No new scheduler, snapshot or frame identifier is
invented. Existing preparation-only staging remains available with its reference
caches supplied by the caller.

| Source field | Original destination and size |
| --- | --- |
| unsigned body halfword0C | accumulator record -> work300,128 bytes |
| signed body halfword1C, unless FFFF | peer body -> work100,256 bytes |
| signed body halfword1E, unless FFFF | candidate body -> work200,256 bytes |
| signed body halfwordB8, unless FFFF | attached body -> work600,256 bytes |
| original job descriptor tag | work59C,4 bytes |

FFFF links leave the corresponding reused bytes untouched. Native invalid bank
indices reject before mutation; the self accumulator index remains unsigned.
The attachment fetch extends to700, beyond the existing680-byte work type. Its
last128 bytes therefore have explicit owned ArenaPlayerReferenceCache storage;
no copy overruns or guessed global work-layout expansion are used. The staging
API updates work/cache transactionally. B500 and9A70 staging writes are disjoint;
accumulator consumption and preparation completion remain their existing steps.

Mode0 attached branch10648..10664 now resets body74 and selects the complete
cached body quadword as the next position. It bypasses11640's unattached78/stage
reset, retains the original delayed visible-position exchange, and preserves the
fourth lane as packed identity/type bits. Candidate-distance math reads XYZ only;
metadata subnormals are not rejected as floating coordinates. Cache identity/type
validation is a native precondition check. Automatic-control/banking paths remain
explicitly blocked instead of replaced.

A connected test registers two real craft, explicitly installs an attachment
link in a fixture, fetches the linked body/accumulator from actual storage, then
runs9A70 consumption/completion ->10290 ->6578 over three updates. It checks both
banks, the metadata lane and the delayed renderer-facing position. The fixture
is not an implementation or demonstration of original pickup/link creation.

## Original-instruction comparison evidence

The private pinned interpreter executes all B500/3108/21F8 instructions, including
actual original transfer setup with native byte-bank reads at the DMA boundary.
Seed B500,10,000 randomized cases: every byte of the complete700-byte output
matches, including reused work, attachment tail, sentinel combinations and tags.
The bank and accumulator fixtures each contain16 records; tag inputs span0..31.
Native out-of-range rejection is covered separately in public tests.

Two controller suites each contain5,000 cases, seed10648. The first has4,992
accepted outputs exact,7 matching contact-effect boundaries and1 explicit
near-axis numeric exclusion. The second adds action/candidate/target-facing
states:4,991 accepted outputs exact,3 matching contact-effect boundaries and6
explicit numeric exclusions. Its2,477 reserve attempts,1,841 publications and123
owner requests match. Accepted comparisons cover the full680-byte context,
reused pending/owner scratch, captured callback payloads and counts. Old position
metadata includes random raw fourth-lane bits. These suites overlap; do not add
them into distinct coverage. Fresh action visual emission is disabled in these
attachment suites; its unchanged four-request path has separate PR #66 evidence.

Original2058/2198/1E090 remain captured transport boundaries for controller
comparison; the public scene test uses recovered native commit code. Interpreter
output matching is scoped evidence, not hardware parity, compiler byte matching
or whole-game equivalence. Original binaries and private harnesses are excluded.
Focused independent source review found no material implementation defect.
Release build and73/73 CTests pass; repository safety and whitespace checks pass.

## Correction and remaining work

Tracing FDFE0's initialized dispatch maps kind18 to2BE0/5558. That consumer reads
player/target bodies and updates its own visual record. It does not write or
detach the target. PR #66's “release requests” name and proposed carry-consumer
priority were wrong. Its original request bytes and controller comparisons remain
valid; the helper/comments and current planning terminology are corrected here.

Next recover the actual original pickup/link and drop/unlink transitions and the
ground-entity carrying update, following player and ground controller state writes
rather than treating visual emission as world mutation. Nonzero snapshot28D4
banking/automatic control, energetic peer contacts, lifecycle/death effects,
original scheduling/snapshot ownership, queue bootstrap and full input adaptation
remain. No graphical original gameplay gate has passed.

Overall complete-game estimate remains about25%, plausible15–35%, with its scope
and limits in completion-assessment.md. This checkpoint removes two concrete
player-loop blockers; that is not a measured whole-game percentage gain. The old
G1 allowance16/32/64 focused hours is uncalibrated and low confidence; discovery
of the mistaken consumer interpretation gives no basis for claiming it shrank.
No reliable whole-game completion date or phase timing is established.
