# Native construction route fields

`decomp/src/arena_routes.cpp` replaces raw construction job offset `0xE48..0x13CC` (job producer PPU `0xFDE54`, binary VA `0x1C5580`). It also recovers the gate visibility expression at job `0x9E8..0xA2C`.

The route routine runs Dijkstra over the fixed 24×24 grid. Its selector at `0xC78` scans cells in ascending order, uses strict comparisons and therefore preserves the first equal-cost cell. Cardinal neighbors are relaxed before diagonal neighbors. A selected low-nibble visibility bit adds 10,000 to a destination; diagonal movement also adds both adjacent cardinal obstruction penalties. Addition order, the original float constants, and strict relaxation comparisons are retained.

Each route byte contains obstruction status in bit 0, predecessor direction in bits 1..3, and saturated floor(cost × float `0x3EAAAAAA`) in bits 4..7. The source byte remains unchanged. The job's caller must still choose the correct active-player pair, supply its visibility bit and commit or discard the candidate grid.

The queue consumer writes the actual owner/opcode/x/z request at descriptor `+0x60..+0x63`. `0xFDBF0` separately refreshes current player controls at `+0x74..+0x77`. These fields are distinct; the SPU reads the queued request directly, without a descriptor-pointer adjustment. Runtime scalar byte/halfword extraction requires accounting for the preferred slot after `rotqby`; interpreting the whole rotated register as the coordinate gives incorrect values.

`apply_arena_grid_placement` now replaces the grid transaction for ordinary supported construction opcodes, gates (25), visibility removal (255) and the grid portion of base relocation (29). It checks occupancy, updates a private candidate, clears route usage, recalculates active opposing-team pairs, rejects obstruction-crossing routes, marks route/corner usage and records Manhattan path lengths. Base relocation routes against the owner's new position and reactivates that player locally; persistence of player coordinates and pair-record state belongs to completion handling. Route bytes use 12 compact directed-pair slots; lengths use the distinct 4×4 pair matrix. Rejection leaves every input grid field unchanged. Entity allocation, resource updates and completion integration remain separate recovery work.

`clear_arena_grid_cell_list` recovers opcode 0: `0xBA8` atomically copies its 128-byte coordinate list and resets the count, then `0x8E8` clears the listed cells' visibility and validates routes. If validation fails, the grid remains unchanged but the coordinate list remains drained. This ordering differs from grid rollback and is preserved in the native boundary. The native list is limited to 63 positions and rejects out-of-grid coordinates before draining; concurrent SPU reservation retries belong to the original transport rather than the single-threaded native state model.

## Verification

A private harness executed the original job routine at `0xE48` using the project's pinned ps3recomp interpreter (`d3ed1a5c946a9c5370b51631e13371a1adf70396`). It staged a 16-byte-per-cell grid, one route-pair record, a valid local stack and a return sentinel. Unexpected stops and channel accesses fail the harness. It did not patch or replace original routine instructions.

Forty deterministic fixtures (seed 9871), including an open grid and randomized obstruction masks, source positions and team bits, produced identical native and original outputs for all **23,040 route bytes**. Original bytes, dependency copies and harness outputs remain private in ignored `generated/spu-oracle/`.

Portable native tests cover open-grid direction/band values observed from the original routine, obstruction penalties, team-specific traversal, gate visibility and native bounds guards. This proves the route-field boundary; it does not establish the entire SPU interpreter as hardware-perfect or complete construction/gameplay.

A second private harness entered the full original job at `0x20`, staged its context, descriptor, grid and twelve pair records, and captured its DMA writes. Sixty fixtures (seed 7911) for ordinary placement, gates and removal matched completion status and every committed 9,344-byte grid snapshot: **26 accepted and 34 rejected**. The fixture used two opposing active players at (2,12)/(21,12); broader player/team/state integration remains to be checked. The pinned interpreter lacks a `rothmi` dispatch case: the private harness adds that case using its existing `spu_rothmi` helper. No original job instruction was changed. Staging mistakes were caught by bounded instruction tracing and corrected before comparison.

Twenty-four additional base-relocation fixtures (seed 3201) matched status and complete grid snapshots: **9 accepted, 15 rejected**. This comparison covers the grid transaction, not persistence of the modified route-pair records.

Twenty-four list-clear fixtures (seed 149) matched completion status and grid writes: **16 accepted, 8 rejected**. The original harness modeled successful GETLLAR/PUTLLC transfers and verified the shared list count was zero on every outcome. Reservation-contention retries and lists with multiple entries require broader oracle coverage.

Thirty-six further placement/gate/base/removal fixtures (seed 4441) varied one to four players, owners, teams and inactive-player flags, and matched original status/grid output: **30 accepted, 6 rejected**. These include open-grid four-way opposing-team routes and same-team skips. A portable regression check confirms skipped pairs preserve their route bytes and lengths while usage masks are cleared.

PPU completion handling at `0xCCC70..0xCCD3C` calls `0xD6088` with commit=1 after successful SPU validation. If that constructor fails, `0xCCD40..0xCCD88` queues opcode 255 to release the reserved cell. Thus normal placement uses both stages: SPU reservation/path validation, then PPU entity/resource construction. `0xCD8E8..0xCD9C8` additionally resets the relocated base's event state and persists base/pair coordinates after successful construction. The map initializer is not the only commit caller.

Primary runtime reference: [pinned SPU interpreter](https://github.com/sp00nznet/ps3recomp/blob/d3ed1a5c946a9c5370b51631e13371a1adf70396/runtime/spu/spu_interp.c).
