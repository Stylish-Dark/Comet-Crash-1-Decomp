# NEXT

This file is retained for compatibility with older project handoffs.

**The authoritative queue is now `WORK_QUEUE.md`.**

Current dependency: run the **Boot Fix 8 jump-table repair** package (ZIP SHA-256 `4e16f475480a3e98037e9b3a21dd5d8bfa8b872bee67d9e68b23d399004163c2`).

The returned Boot Fix 6 log contains an earlier `[ppu] unresolved indirect call -> 0x00052DF4` before the later allocator abort. Static analysis proves `0x00052DF4` is the fifth target of the seven-entry inline signed-relative switch table after `0x0005220C: bctr`, not a standalone function. PR #15 repairs the pinned lifter so that switch is emitted as in-function case labels, including `case 0x00052DF4u: goto loc_00052DF4;`.

Boot Fix 8 retains every Boot Fix 7 pointer/allocator diagnostic. On the next run:
- first confirm the unresolved `0x00052DF4` dispatch is gone;
- if execution advances, follow the next concrete runtime signal;
- if the `0x00000140` bad free remains, use the retained parse-pointer markers to identify the exact remaining corruption boundary.

Boot Fix 8 EXE SHA-256: `b945f1070db2b2bd808ffcc19e02c1fda97eb2658d36e349a1682d1801dd121c`.
