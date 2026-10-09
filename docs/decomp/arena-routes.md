# Native construction route fields

`decomp/src/arena_routes.cpp` replaces raw construction job offset `0xE48..0x13CC` (job producer PPU `0xFDE54`, binary VA `0x1C5580`). It also recovers the gate visibility expression at job `0x9E8..0xA2C`.

The route routine runs Dijkstra over the fixed 24×24 grid. Its selector at `0xC78` scans cells in ascending order, uses strict comparisons and therefore preserves the first equal-cost cell. Cardinal neighbors are relaxed before diagonal neighbors. A selected low-nibble visibility bit adds 10,000 to a destination; diagonal movement also adds both adjacent cardinal obstruction penalties. Addition order, the original float constants, and strict relaxation comparisons are retained.

Each route byte contains obstruction status in bit 0, predecessor direction in bits 1..3, and saturated floor(cost × float `0x3EAAAAAA`) in bits 4..7. The source byte remains unchanged. The job's caller must still choose the correct active-player pair, supply its visibility bit and commit or discard the candidate grid.

The queue consumer writes the actual owner/opcode/x/z request at descriptor `+0x60..+0x63`. `0xFDBF0` separately refreshes current player controls at `+0x74..+0x77`. These fields are distinct; the SPU reads the queued request directly, without a descriptor-pointer adjustment. Runtime scalar byte/halfword extraction requires accounting for the preferred slot after `rotqby`; interpreting the whole rotated register as the coordinate gives incorrect values.

`apply_arena_grid_placement` now replaces the grid transaction for ordinary supported construction opcodes, gates (25), and visibility removal (255). It checks occupancy, updates a private candidate, clears route usage, recalculates active opposing-team pairs, rejects obstruction-crossing routes, marks route/corner usage and records Manhattan path lengths. Route bytes use 12 compact directed-pair slots; lengths use the distinct 4×4 pair matrix. Rejection leaves every input field unchanged. Cell-list atomic removal (opcode 0), base relocation (29), entity allocation, resource updates and completion integration remain separate recovery work.

## Verification

A private harness executed the original job routine at `0xE48` using the project's pinned ps3recomp interpreter (`d3ed1a5c946a9c5370b51631e13371a1adf70396`). It staged a 16-byte-per-cell grid, one route-pair record, a valid local stack and a return sentinel. Unexpected stops and channel accesses fail the harness. It did not patch or replace original routine instructions.

Forty deterministic fixtures (seed 9871), including an open grid and randomized obstruction masks, source positions and team bits, produced identical native and original outputs for all **23,040 route bytes**. Original bytes, dependency copies and harness outputs remain private in ignored `generated/spu-oracle/`.

Portable native tests cover open-grid direction/band values observed from the original routine, obstruction penalties, team-specific traversal, gate visibility and native bounds guards. This proves the route-field boundary; it does not establish the entire SPU interpreter as hardware-perfect or complete construction/gameplay.

A second private harness entered the full original job at `0x20`, staged its context, descriptor, grid and twelve pair records, and captured its DMA writes. Sixty fixtures (seed 7911) for ordinary placement, gates and removal matched completion status and every committed 9,344-byte grid snapshot: **26 accepted and 34 rejected**. The fixture used two opposing active players at (2,12)/(21,12); broader player/team/state integration remains to be checked. The pinned interpreter lacks a `rothmi` dispatch case: the private harness adds that case using its existing `spu_rothmi` helper. No original job instruction was changed. Staging mistakes were caught by bounded instruction tracing and corrected before comparison.

Primary runtime reference: [pinned SPU interpreter](https://github.com/sp00nznet/ps3recomp/blob/d3ed1a5c946a9c5370b51631e13371a1adf70396/runtime/spu/spu_interp.c).
