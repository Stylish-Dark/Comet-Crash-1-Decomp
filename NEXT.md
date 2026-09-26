# NEXT

This file is retained for compatibility with older project handoffs.

**The authoritative queue is now `WORK_QUEUE.md`.**

Current dependency: run the **Boot Fix 8 jump-table repair — static runtime** package (ZIP SHA-256 `bbfc535577088da76e47c2e52b556ca96648f5a7f55f2b1263b684bccd42c2a9`).

The returned Boot Fix 6 log contains an earlier `[ppu] unresolved indirect call -> 0x00052DF4` before the later allocator abort. Static analysis proves `0x00052DF4` is the fifth target of the seven-entry inline signed-relative switch table after `0x0005220C: bctr`, not a standalone function. PR #15 repairs the pinned lifter so that switch is emitted as in-function case labels, including `case 0x00052DF4u: goto loc_00052DF4;`.

Static comparison now supplies a strong causal mechanism for the old allocator symptom: the missed switch made `func_0005207C` return through the unresolved dispatcher while its 0x210-byte guest stack frame was still active, leaving guest `r1` 0x210 bytes low and callee-saved state unrestored. The unresolved path returns through `func_00054400` to `func_0002D868` at `0x0002DA58`, which also appears in the later allocator-abort chain. Runtime Boot Fix 8 is still required to confirm that repairing the switch removes the downstream `0x140` free.

PR #18 adds an independent source-ELF structural gate: the exact title contains 73 recognized inline signed-relative `bctr` tables and all 73 are present in the repaired generated lift, including `0x0005220C`; zero are missing.

Boot Fix 8 retains every Boot Fix 7 pointer/allocator diagnostic. On the next run:
- first confirm the unresolved `0x00052DF4` dispatch is gone;
- if execution advances, follow the next concrete runtime signal;
- if the `0x00000140` bad free remains, use the retained parse-pointer markers to identify the exact remaining corruption boundary.

Boot Fix 8 static-runtime EXE SHA-256: `ac0de506db1f9ee63c3968307f357da65cbbe17be6288a10ea3522147797159e`.

Current user-ready Boot Fix 8 static-runtime ZIP SHA-256: `bbfc535577088da76e47c2e52b556ca96648f5a7f55f2b1263b684bccd42c2a9`.
