# NEXT

This file is retained for compatibility with older project handoffs.

**The authoritative queue is now `WORK_QUEUE.md`.**

Current dependency: run the **terminal-guard Boot Fix 7** build (ZIP SHA-256 `f7efd390fd80631b2925e5cc645dc1a72372b8281199b1b57ed6d222e69ddf86`) and capture the first pointer-lifetime failure marker. The watcher covers all 35 live-pointer call boundaries plus terminal pre-store/pre-free checks. Static control-flow review has ruled out the two visible frees as a simple double-free: the path reaching `0x00130418` skips the earlier `0x001301A8` free. Boot Fix 6 proved that `mspace_malloc`/memalign do not return the invalid `0x140` value. Boot Fix 7 now distinguishes:
- `[COMET-PARSE-REG-CLOBBER]` — saved allocation register changed;
- `[COMET-PARSE-SP-CHANGE]` — caller stack pointer drifted;
- `[COMET-PARSE-SLOT-CHANGE]` — caller `sp+0x84` allocation slot was overwritten.

Use the first marker's exact `site=0x...` to patch the offending function/path, then rebuild and continue.
