# Native entity banks and cell records

`insert_arena_entity` recovers `0xCA514..0xCA7E4`, including the single-index FIFO
operation requested from `0x130A1C`. The caller supplies bootstrap's free-index
order and bank capacity. The original pool holds a head, capacity, available
count and a ring of 32-bit indices; the native deque preserves its logical FIFO
order. Bootstrap initialization and index return/recycling are not recovered
here.

Both cell lists must have room before allocation. The selected index consumes
one pool entry. The 256-byte template is copied to both entity banks, then the
index's low halfword is stored at slot +0x0C. The native boundary rejects bad
indices instead of allowing truncation or out-of-bank writes. Its full-cell
guard uses count >=126, extending the original count ==126 guard to malformed
native state.

Cell records are eight bytes:

| Offset | Field |
| --- | --- |
| 0..1 | Entity index, big-endian halfword |
| 2 | Entity byte +0x0E |
| 3 | Supplied marker; CA514 uses 40 |
| 4..6 | Packed x/y/z relative position |
| 7 | Entity packed owner/stage byte +0x33 |

`pack_arena_cell_entry` replaces `0x1117F0..0x1119F0`. It truncates x/z toward
zero, clamps them to the configured extent and reports coordinate changes.
Relative position uses the supplied cell origin `(x-.5, -.5, z-.5)`, subtracts
in float, clamps each component to [0,2], multiplies by 100 and truncates to a
byte. Native invalid extents, nonfinite positions and values outside the safe
32-bit conversion range throw before writes. The fourth position word is not
treated as a float by the native packer; it holds index/quantity/type.

Both original list pointers are selected **before** packing. The first call
can change x/z; the second uses the adjusted coordinates but writes to its
previously selected list. Consequently the two packed records may differ if a
template position differs from its supplied cell. Records append in order.
Afterward the summary halfword at the **final** coordinate increments with
16-bit wrap. The native `read_bank` chooses which packing call occurs first;
the other bank receives the second entry.

`resolve_arena_storage_target` applies recovered CA2E0 visibility, exclusion-mask
and packed stage checks to the selected bank's ordered records, then reads type
from that bank's entity slot. Newly constructed opcode-21 stage-4 entries remain
excluded, matching the existing resolver. Simulation readiness transitions
remain to be recovered; this boundary does not mark a construction ready early.

The construction integration test now executes the real opcode-21 constructor
and insertion boundary through route reservation/completion. It verifies both
entity banks, both cell lists, the pool count, summary counter and resource debit.
This is still a boundary test; it is not wired into interactive viewer gameplay.

## Verification

Portable tests cover reused templates, FIFO indices, both-bank copies, stored
IDs, entry append order, cell fullness, empty-pool rejection, counter wrap, changed
coordinates, selected-bank lookup and exclusion masks.

The private decoded-instruction evaluator also executes `0x1117F0..0x1119F0`
from the user-owned original ELF. The unrecognized Capstone word at `0x111928`
decodes as `lvlx v0,0,r7`, supplying the scale 100 constant. One thousand fixtures
(seed 1170) matched all eight entry bytes, final x/z and returned change status,
across varying positions, cell coordinates, extents, metadata, flags and markers.
The evaluator's VMX multiply-add path asserts the encountered addend is zero;
this proves this bounded helper, not general VMX emulation. The full CA514/pool
routine has source-anchored native tests, rather than a full-routine oracle.

Together with the preceding initializer/constructor checks there are 2,500
decoded-original entity fixtures. Proprietary instructions and harness files
remain private under ignored `generated/ppu-oracle/`.
