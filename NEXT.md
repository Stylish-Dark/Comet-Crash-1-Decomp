# NEXT

This file is retained for compatibility with older project handoffs.

**The authoritative queue is now `WORK_QUEUE.md`.**

Current dependency: run **Boot Fix 7** and capture the first pointer-lifetime failure marker. Boot Fix 6 proved that `mspace_malloc`/memalign do not return the invalid `0x140` value. Boot Fix 7 now distinguishes:
- `[COMET-PARSE-REG-CLOBBER]` — saved allocation register changed;
- `[COMET-PARSE-SP-CHANGE]` — caller stack pointer drifted;
- `[COMET-PARSE-SLOT-CHANGE]` — caller `sp+0x84` allocation slot was overwritten.

Use the first marker's exact `site=0x...` to patch the offending function/path, then rebuild and continue.
