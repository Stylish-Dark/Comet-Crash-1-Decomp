# Entity construction and completion

The native construction path now has a synchronous boundary for the recovered
funded completion flow: pop one request, run the FDE54 grid transaction, call
the supplied entity constructor, debit resources on success, and enqueue opcode
255 on constructor failure. Reservation remains in place until that later
release job executes. Route rejection leaves the grid unchanged and does not
call the constructor. Empty and malformed requests have explicit native results.

`process_next_arena_construction` uses the completion arguments at
`0xCCD18..0xCCD28`: commit, occupancy override, and full opcode cost. The caller's
temporary context flags are restored. The constructor callback owns allocation
and cell-list insertion; a real opcode-21 initializer and CA514 insertion are
exercised through this boundary in the portable integration test. See
`arena-entity-storage.md`. This is not yet the running viewer's
gameplay loop. The owner-control-dependent insufficient-funds notification path
at `0xD6654`, completion retry scheduling, and remaining constructor types are
not integrated. The native funded boundary rejects insufficient resources and
queues release. Queue overflow reports failure rather than overwriting entries.

Base completion persists the new route-player base and clears inactivity. At
`0xCD8E8..0xCD928`, the original calls `0xF0B00` and saves x/z. `0xF0B00` writes
player +0x23A=1 and +0x42/+0x43=0; it does **not** clear the event heap. Native
control fields retain their offset names. Pair routing uses player coordinates
directly, eliminating the original pointer-bearing pair-record copy. Original
`0x111658` updates target coordinates and compact target indexing in those
records. Other base-specific world flags remain separate recovery work.

## Shared initializer

`initialize_arena_entity_core` replaces `0xFECFC..0xFEE10`. Its 128-byte big-endian
core can be used inside the original 256-byte entity bank. It preserves bytes
the original does not write, including the fourth words of the position and
+0x40 vectors. The position's last two bytes are subsequently replaced by
quantity/type at +0x0E/+0x0F. Orientation becomes (0,0,0,1).

Owner/stage packing at +0x33 is `((owner & 7) << 5) | ((stage & 7) << 2)`.
The flags at +0x60 use the **unmasked** stage byte comparison: OR 2 for stage
0..3, otherwise OR 6. Quantity converts its signed 64-bit bit pattern to double,
rounds to float, then multiplies by original float `0x3C23D70A` for +0x78.
The four constructor-supplied scalars retain destination-offset names because
their gameplay meanings have not all been established.

`reset_arena_entity_upgrade_core` replaces `0x12869C..0x1286D4`: clear flag bit
24, set bits 3/7, set +0x74 to float `0x38D1B717`, clear +0x78 and both counters,
store level at +0x6E and clear +0x0E. `settle_arena_entity_upgrade_core` replaces
`0x128C9C..0x128CE0`: add the unsigned halfword counters with 16-bit wrap, clear
the second, clear flag bits 3/4, set bit 24, set +0x74=0/+0x78=1 and restore
+0x0E from +0x7C. Neither transition changes packed owner/stage at +0x33.

## Opcode 21

`initialize_arena_type21_entity` replaces the complete entity-slot write path
`0xD7138..0xD72E4`, including the shared initializer and upgrade reset. It
initializes model slot 11, stage 4, quantity 35, constructor flags, and the
constructor-specific +0x80..+0xEF region. Scalar tuning values read from mutable
bootstrap tables are explicit inputs; native defaults are not claimed as the
original game's tuning. The constructor leaves +0x90..+0x9F and +0xF0..+0xFF
untouched, as well as other unwritten core fields. This does not establish
allocation, simulation readiness, weapon behavior or the other constructors.

## Verification

Portable tests cover preserved bytes, owner/stage masks, signed quantity
conversion, counter wrap, constructor fields, real opcode-21 construction through
the reservation/completion boundary, failed-constructor release ordering, full
cost, route rejection, and base relocation with an existing event heap.

A private instruction evaluator decoded the original initializer and opcode-21
constructor directly from the exact-title ELF using Capstone. It evaluated only
their encountered integer, memory, float, VMX-selection and branch instructions;
unexpected instructions fail. There were no substituted original instructions.
It matched all 128 output bytes for 1,000 initializer fixtures (seed 1187) and
all 256 bytes for 500 opcode-21 fixtures (seed 2109), including reused-slot bytes,
signed tuning values, stage boundary values, and large signed quantity patterns.
This bounded evaluator is an additional recovery check, not a hardware-perfect
PPU emulator or evidence of a complete game. Proprietary code and private
harness files remain under ignored `generated/ppu-oracle/`.

## Opcode 20 continuation

`initialize_arena_type20_entity` replaces D7090..D7134: model 17, stage 4, quantity 40, constructor flags 0001C200, scalar +3C=0, +58=200, the signed mutable-table scalar/ratio, +10=2, +8C=0 and the level-1 upgrade reset. Other bytes beyond the core and +8C remain untouched. Five hundred decoded-original fixtures (seed 2009) matched all 256 bytes. Together with opcode 21, core and packed-cell checks there are now 3,000 entity fixtures. The remaining constructor types are still unimplemented.
