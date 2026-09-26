# NEXT

This file is retained for compatibility with older project handoffs.

**The authoritative queue is now `WORK_QUEUE.md`.**

Current dependency: run the **Boot Fix 8 jump-table repair — strict dispatch** package (ZIP SHA-256 `ee38762bfe0c4490bfcdc561b8d3aebf32adaae6997b0521a04994b2c0f6d344`).

The returned Boot Fix 6 log contains an earlier `[ppu] unresolved indirect call -> 0x00052DF4` before the later allocator abort. Static analysis proves `0x00052DF4` is the fifth target of the seven-entry inline signed-relative switch table after `0x0005220C: bctr`, not a standalone function. PR #15 repairs the pinned lifter so that switch is emitted as in-function case labels, including `case 0x00052DF4u: goto loc_00052DF4;`.

Static comparison now supplies a strong causal mechanism for the old allocator symptom: the missed switch made `func_0005207C` return through the unresolved dispatcher while its 0x210-byte guest stack frame was still active, leaving guest `r1` 0x210 bytes low and callee-saved state unrestored. The unresolved path returns through `func_00054400` to `func_0002D868` at `0x0002DA58`, which also appears in the later allocator-abort chain. Runtime Boot Fix 8 is still required to confirm that repairing the switch removes the downstream `0x140` free.

PR #18 adds an independent source-ELF structural gate: the exact title contains 73 recognized inline signed-relative `bctr` tables and all 73 are present in the repaired generated lift, including `0x0005220C`; zero are missing.

PR #19 makes any future unresolved aligned title-text target fatal after normal dispatcher recovery. Instead of returning with a stale guest stack/register set, the runtime logs the exact target, dumps the guest stack, and exits immediately.

Boot Fix 8 retains every Boot Fix 7 pointer/allocator diagnostic. On the next run:
- first confirm the unresolved `0x00052DF4` dispatch is gone;
- if execution advances, follow the next concrete runtime signal;
- if the `0x00000140` bad free remains, use the retained parse-pointer markers to identify the exact remaining corruption boundary.

Boot Fix 8 strict-dispatch EXE SHA-256: `4b519db0656e0e1c5fb64739c4e75d7ca2987cce6845321e0597246148c2ee39`.

Current user-ready Boot Fix 8 strict-dispatch ZIP SHA-256: `ee38762bfe0c4490bfcdc561b8d3aebf32adaae6997b0521a04994b2c0f6d344`.
