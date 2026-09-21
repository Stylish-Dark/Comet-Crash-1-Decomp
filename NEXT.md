# NEXT

## Immediate priority — first native Windows boot

1. Run:

   ```bat
   scripts\build_and_run.cmd "D:\Games\Comet Crash"
   ```

2. Preserve the generated `logs\boot-YYYYMMDD-HHMMSS.txt`.

3. Determine the last successful `[boot-stage]` and the first concrete failure after it.

4. Classify the blocker using evidence, in roughly this order:
   - process / VM / PPU dispatch;
   - filesystem / VFS / title-data paths;
   - missing or incorrect HLE behavior;
   - GCM / RESC / RSX;
   - SPURS / SPU dispatch;
   - synchronization / HOTREAD;
   - audio;
   - input or host integration.

5. Fix only the evidenced blocker, add a focused regression test where possible, then checkpoint continuity before the next run.

## After first visible output

- Verify title/menu rendering and stable frame presentation.
- Verify original controller input before judging mouse behavior.
- Enter a mission and verify simulation, tower placement/selection, pause, exit and save/load behavior.
- Restore/fix audio/SPU behavior if it was merely bypassed or still partial.

## Deferred until vanilla gameplay is stable

- direct absolute mouse/world-pointer injection;
- refresh-rate unlock / simulation decoupling;
- broader graphics options and aspect-ratio work;
- cosmetic/remaster changes;
- network-service restoration beyond local-play requirements.
