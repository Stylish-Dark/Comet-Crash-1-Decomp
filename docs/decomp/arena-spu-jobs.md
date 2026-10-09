# Raw arena SPU jobs

The existing two-image SPU inventory counted embedded ELF files. It did not include these six raw job binaries. They have SPURS job headers and no ELF magic. This is a separate category of original executable code, not six newly discovered ELF files.

Exact input SHA-256: `3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.

| PPU descriptor producer | Job code VA range | Bytes |
|---|---|---:|
| 0xFDC5C | 0x1C8000..0x1CF400 | 29,696 |
| 0xFDD5C | 0x203880..0x215D00 | 74,880 |
| 0xFE1C0 | 0x1CF580..0x1CFF10 | 2,448 |
| 0xFE5D8 | 0x1CFF80..0x1F6BF0 | 158,832 |
| 0xFDFE0 | 0x1F6C00..0x203810 | 52,240 |
| 0xFDE54 | 0x1C5580..0x1C8000 | 10,880 |

Ranges are end-exclusive. Producer functions load start/end pointers from module TOC 0x241DB8 and call job-header helper 0x111D98. Exact TOC slots, hashes and filenames are in `arena-spu-jobs.json`. Job generators configure different input buffers, output buffers and chunking; their full gameplay roles must be recovered from the SPU bodies rather than inferred from their sizes.

## Construction consumer

PPU `0xCAB90` is the construction ring consumer/orchestrator:

- Root +0x2D2900 is a 256-record ring; +0x400 head, +0x404 tail, +0x408 count.
- An outstanding request at root +0x2CC880 prevents another submission until completion. Its status +0x10 is examined first.
- `0xCAC50..0xCACD0` takes one ring record, decrements count, wraps head at 256 and copies its player/opcode/x/z bytes to root +0x2CA360.
- It refreshes the job at root +0x2CA300 through `0xFDBF0`, queues its address with `cellSpursAddUrgentCommand` (`0xCACEC`) and runs the chain at root +0x3480 (`0xCACF8`).
- `0xCFD88..0xCFDCC` proves that job descriptor is built by `0xFDE54`, with code range 0x1C5580..0x1C8000.
- On completion, it copies status/player/opcode/x/z into root +0x2D2D0C..0x2D2D11 and can copy back a 0x2480-byte visibility-grid snapshot (`0xCAC04..0xCAC40`, `0xCAD30`).

This means selector-0 pending handling in `0xCC9CC` only validates and submits. Immediate native entity construction would skip the original asynchronous job's commit logic. The next native replacement must recover this 10,880-byte job's command dispatch and state writes.

The regular simulation schedules other jobs. `0xDAAC8` prepares snapshot pointers/delta and calls firmware trampoline `0x1B6A78`, whose NID 0xD5D0B256 is `cellSpursJobGuardNotify`. Name verification used the primary RPCS3 implementation's NID hashing and registered cellSpurs names; it is not a guess based on the call arguments.

## Extraction and private inspection

Run `python tools/extract_arena_spu_jobs.py EBOOT.ELF private-output-directory`. The extractor validates every input range/header before writing any payload. It records metadata and original raw bytes only in the chosen private directory. Do not commit the extracted binaries or disassembly.

Private decoding used the project's pinned ps3recomp SPU disassembler (`d3ed1a5c946a9c5370b51631e13371a1adf70396`). The decoder and any wrapper/lift output remain private tools, not native game-domain code. ABI staging and vector/scalar interpretation need original/runtime cross-checks before a source replacement is accepted.

Primary external verification sources: [RPCS3 cellSpurs](https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/Cell/Modules/cellSpurs.cpp), [RPCS3 NID generation](https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/Cell/PPUModule.cpp), [pinned ps3recomp SPU decoder](https://github.com/sp00nznet/ps3recomp/blob/d3ed1a5c946a9c5370b51631e13371a1adf70396/tools/spu_disasm.py).
