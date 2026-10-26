# Shared entity finalization and delayed removal

Latest continuation: [attachment states](original-attachment-states.md) recovers3940's immediate return for nonzero body7F. First-display allocation remains open.

## Source and connected boundary

Reference: private NPEB00142 v1.00 EBOOT.ELF SHA256
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`;
FE5D8 SPU job SHA256
`9aca1695e6409668d9627ce865ad8008119f1c9e42d81a6106369f5a95e1872f`.
Addresses below are local-store addresses in that job. No original binary or
instruction dump is distributed with this change.

Type1 wrapper12B88 calls9A70 preparation,10290 controller, then6578 withr4=1.
This change recovers the shared finalization dependency and exposes it through
ArenaConstructionSimulation::finalize_player_craft for an already registered
original body. It does not claim the missing preparation/controller has run.

| Source | Recovered behavior |
| --- | --- |
| 6578..6724 | Clear flags bit1; live health/change gate; XYZ-only staged position/velocity commit; handled-removal early return; otherwise statistics removal and tombstone construction |
| 3400..3488 ->2DF8..3060 | Fixed kind63/model40 constructor, position(-10,-10,0), zero velocity/owner/quantity/stage, scalars100/0/1/100 and wrapping original serial+3 at+80 |
| 790 wrapper,874..894 | Update packed cell marker; final kind63 bypasses new-position cell packing and retains old membership |
| E00..E84 | Collect kind63 only when deadline equals the current original serial; do not use greater-than-or-equal or elapsed time |
| Completed-phase owned adapter | Explicit ordered identity return to the existing FIFO; retain slot bytes for original reused-memory construction |

The fixed constructor reuses the recovered shared core field writes, verified
against these fixed SPU arguments. Identity bytes+0C/+0D, packed fourth velocity
lane and unwritten reused bytes remain intact. Source29F8 returns pool identities
without clearing entity memory; the owned release adapter preserves those bytes.
The existing immediate native retire API is unchanged.

The removal path calls the recovered statistics operation with adding=false,
count_active=true and context+596. The original ignores that operation's return,
including type1 unsupported accounting. Already-handled removal must not unregister
again or recreate the deadline. Publication stages changes and rejects invalid
native storage without partially applying statistics.

## Explicit remaining boundaries

- 3940 change effects remain unrecovered. Health difference greater than source
  bits38D1B717 or changed59A lowbit, when flags bit2 is set, returns
  NeedsChangeEffects without writes. This does not silently omit effects.
- r4=0 requires6568/6260 spatial work; return NeedsSpatialRecovery without writes.
  Type1 wrapper passes1. These conditions are not whole-game error handling.
- The subtraction uses round-toward-zero on finite normal inputs/zero. Nonfinite
  and subnormal values are outside this native supported domain. No original
  hardware/subnormal equivalence or new interpreter differential claim is made.
- Publication to both owned banks is a synchronous native adapter. Original DMA,
  cross-job scheduling, completion ordering and serial ownership remain open.
  Collection enumerates native cell/list order; that is not claimed as the
  original cross-job ordering. Caller supplies the completed batch/order/serial.
- No automatic render-frame deadline, inferred30/60Hz tick, F3 preview integration,
  original controller/action/carry completion or complete ship death lifecycle.
  Player/root links are retained; source respawn/link cleanup remains unrecovered.

## Validation

The focused test covers live registered-body movement, packed metadata, bank/cell
publication, original fixed constructor fields, statistics removal once, retained
kind63 membership including already-handled removal, exact serial admission,
32-bit wrap, delayed FIFO release, preserved reused memory, and rejection of stale
or duplicate batches. It also checks explicit unrecovered branches do not write.
The new target participates in the existing headless CTest suite. These are
source-traced implementation/integration checks, not whole-game matching proof.
