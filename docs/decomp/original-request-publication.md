# Original counted-request publication

This continues [original preparation completion](original-preparation-effects.md).
The recovered3510 counted-request constructor can now feed a real native record
pool through recovered1E090 publication and2AD8 FIFO allocation. This removes
one shared dependency of preparation and the still-unrecovered10290 controller.
Published requests still need their original downstream consumer/scheduler.

## Provenance and behavior

NPEB00142 v1.00 ELF SHA256:
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
FE5D8 job SHA256:
`9aca1695e6409668d9627ce865ad8008119f1c9e42d81a6106369f5a95e1872f`.
No original executable, disassembly, asset or private harness is distributed.

| Original path | Recovered behavior |
| --- | --- |
| 1E094..1E124 | Snapshot+27F0 record pool and+2814 free queue |
| 1E130..1E148 | Mode0 starts at descriptor byte+0E with zero phase increment |
| 1E340..1E36C | Other modes start at zero; original reciprocal estimate/refinement computes255/count with round toward zero |
| 1E180..1E19C | Allocate min(remaining,32), passing reserve128 to2AD8 |
| 2B18..2B2C | Fail allocation if count+reserve exceeds available |
| 2B30..2C88 | Read original FIFO identities across wrap; advance head modulo ring capacity; subtract the allocation from all four availability words |
| 1E1E8..1E29C | Copy full128-byte descriptor, replacing halfword+0C with the allocated identity |
| 1E390..1E478 | Distributed mode also writes truncated phase to bytes+0E and+2E, advancing phase continuously across batches |
| 1E2A0..1E2E4 | Publish each batch to allocated record slots; failure of a later allocation leaves previous batches intact |
| 1E2F0 | Return zero even on exhaustion;3510 ignores that result and clears its counter |

`publish_arena_request` accepts caller-owned `ArenaRequestPublicationQueue` and
an existing `ArenaCounterEmissionRequest`. It returns publication/exhaustion
bookkeeping separately from the original zero return, so callers can observe
what happened without fabricating success or making the source retry lost work.
The descriptor, free-ring entries and unallocated records remain untouched.
Adjacent availability lanes+34/+38/+3C retain the original unsigned subtraction;
they are not discarded as presumed padding.

## Necessary platform adaptation

Original2918/29A8 acquire/release a shared128-byte queue lock; original2AD8 uses
DMA to read FIFO identities, and1E090 uses DMA-list writes to publish records.
The native API uses caller-serialized ownership and synchronous vector writes.
It does not run a PS3 instruction interpreter or retain PS3 effective pointers.
The source lock word and its three zeroed neighboring words have no native
representation. Queue/pool ownership and original bootstrap order are supplied
by the caller; no ascending reset policy or frame scheduling is invented.

Native validation rejects malformed queue state before writes. Supported rings
have a capacity divisible by4, matching the source's aligned ring transfer;
available entries must address the supplied record pool. Indices are bounded
by the original16-bit stored identity (at most65536 slots). This is not a claim
about arbitrary corrupted source queue headers or concurrent reentrant access.

The existing preparation emitter can be bound directly to this publisher. The
integration test sends actual completed preparation's counted request into the
pool, and separate3510 tests verify ordered denomination requests and ignored
exhaustion. Publishing a request does not mean its original downstream entity
creation/consumption has run. The F3 preview does not choose or run this queue.

## Validation scope

The private comparison executes original1E090,2AD8 and2918/29A8 instructions in
the pinned ps3recomp interpreter atd3ed1a5c. Synthetic DMA and successful atomic
lock operations complete synchronously. This establishes serial output parity,
not hardware concurrency, DMA timing, compiler matching or whole-game fidelity.
All10,000 cases matched exactly;7,481 exhausted the available reserve and472
wrapped the FIFO. The compared synthetic image includes the complete queue header, FIFO, record
pool and untouched memory; initial lock/neighbor words are zero. Cases vary FIFO
order/head, reserve/full/partial exhaustion, record metadata, phase mode and
counts including32-boundary cases and UINT32_MAX.

Three public original-output fingerprints cover odd phase counts33/65/129,
nonascending identities, wrapped head500 and all512 records. Direct tests
also cover reserve128, partial32-record publication, preserved bytes, invalid
state and the connected preparation-to-pool path. Strict FP flags apply to the
new arithmetic on GCC/Clang and MSVC. Release native build and all71 CTests
passed; focused review found no publisher logic blocker.

## Remaining work

Recover full10290 player controller/actions/carry, original queue bootstrap and
downstream request consumption, and snapshot/job ownership. Existing3940/6260
and numeric boundaries remain explicit. This milestone supplies publication;
it does not turn selected preview flight into the full original controller.
