# NEXT

## Immediate priority — first native Windows boot

1. Use the recovered advanced source snapshot (commit state `c0089d4`).
2. Run:
   ```bat
   scripts\build_and_run.cmd "D:\Games\Comet Crash"
   ```
3. Preserve `logs\boot-YYYYMMDD-HHMMSS.txt`.
4. Determine the last successful `[boot-stage]` and the first concrete failure after it.
5. Classify the blocker using evidence: VM/PPU → VFS → HLE → GCM/RESC/RSX → SPURS/SPU → synchronization/HOTREAD → audio → input/host integration.
6. Fix only the evidenced blocker, add a focused regression test where possible, then checkpoint continuity before the next run.

## Repository recovery task

The advanced checkpoint's source files still need to be replayed fully into the current GitHub working tree. Until that reconciliation is complete, use the persistent recovery archives named in `CONTINUITY.md` rather than stale scaffold files.

## After first visible output

- verify title/menu rendering and stable frame presentation;
- verify original controller input;
- enter a mission and verify simulation, tower placement/selection, pause, exit and save/load;
- resolve any remaining required audio/SPU behavior.

## Deferred

- direct absolute mouse/world-pointer injection;
- refresh-rate unlock / simulation decoupling;
- broader graphics/aspect-ratio work;
- cosmetic/remaster changes;
- network-service restoration beyond local-play requirements.
