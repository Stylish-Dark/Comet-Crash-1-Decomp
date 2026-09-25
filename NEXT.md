# NEXT

This file is retained for compatibility with older project handoffs.

**The authoritative queue is now `WORK_QUEUE.md`.**

Current external dependency: run the ready-to-run **Boot Fix 6** package and capture the first `[COMET-MALLOC-LOW]` / `[COMET-MALLOC-STATE]` evidence. The diagnostic identifies the exact `mspace_malloc` basic block that produced the invalid `0x140` user pointer (structurally a bogus chunk at `0x138`). After that, trace and fix the exact allocator metadata writer.
