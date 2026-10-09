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
and cell-list insertion; all seven recovered structure initializers and CA514 insertion are
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
conversion, counter wrap, constructor fields, all seven recovered structures through
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

`initialize_arena_type20_entity` replaces D7090..D7134: model 17, stage 4, quantity 40, constructor flags 0001C200, scalar +3C=0, +58=200, the signed mutable-table scalar/ratio, +10=2, +8C=0 and the level-1 upgrade reset. Other bytes beyond the core and +8C remain untouched. Five hundred decoded-original fixtures (seed 2009) matched all 256 bytes. The continuation below extends the recovered set.

## Generic structure dispatch: 22, 23, 24, 26 and 28

`initialize_arena_structure_entity` dispatches opcodes 20, 21, 22, 23, 24,
26 and 28. Unsupported requests return false and preserve the whole slot.
The new arms cover D72E8..D72E4 (22, returning through the shared tail),
D7404..D72E4 (23), D7590..D7740 (24), D788C..D7990 (26), and
D683C..D6940 (28), including FECFC and 12869C. Fields that the original
leaves unwritten remain intact, including +8C and +A9 for 26/28.
The original comparison caught a significant difference: 26/28 retain
FECFC's +10=32, while 20..24 overwrite it with 2. Opcode 24 retains
pre-switch f30=0.5 for +E4/+EC. Upgrade reset levels are 1 for 20..24,
2 for 26 and 5 for 28.

The common signed halfword inputs read table TOC -6DB4 and -6DCC at
2*(opcode-20). Other mutable inputs remain explicit:

| Opcode | first_byte | second_byte | scalar_84 |
| --- | --- | --- | --- |
| 20 | unused | unused | unused |
| 21 | TOC -6DA8, byte 0 | TOC -6D80, byte 0 | unused |
| 22 | TOC -6D80, byte 5 | unused | unused |
| 23 | TOC -6D48, byte 0 | TOC -6D80, byte 10 | unused |
| 24 | TOC -6D80, byte 15 | TOC -6D64, byte 0 | unused |
| 26 | unused | unused | TOC -6D50, float +0 |
| 28 | unused | unused | TOC -6D50, float +10 |

Here “TOC” means a pointer loaded from that TOC displacement; table offsets
are hexadecimal. A fresh mixed test constructs all seven opcodes through
routing and completion, verifies ascending allocated indices, equal entity
banks, both ordered cell lists and summaries, exhausts the pool and checks
400 minus the original total cost of 355 equals 45.

The private decoded-instruction evaluator now matches 500 fixtures for each
of the seven constructor arms (seed 2634), checking all 256 bytes with random
reused-slot contents, owners, grid positions and signed mutable tuning.
Together with the 1,000 core and 1,000 packed-cell fixtures, this batch covers
5,500 comparisons. These are bounded instruction-evaluator checks, not
original hardware execution. Gate opcode 25, base 29 and terrain 9 remain
separate recovery work; none of these checks establishes playable simulation.
