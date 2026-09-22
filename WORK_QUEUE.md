# WORK QUEUE

Work units are intentionally bounded so one worker can investigate, integrate, document and commit without approaching the execution limit.

## Current

[ ] **Capture first native Windows boot evidence**
- Input: supported NPEB00142 v1.00 extracted game directory.
- Run `scripts\build_and_run.cmd "<game folder>"`.
- Preserve `logs\boot-*.txt`.
- Confirm the boot-log footer contains `suspected_subsystem` and `triage_rationale`; the normal run path now writes them automatically. `python tools/boot_triage.py <boot-log>` remains available for manual re-analysis.
- Success for this work unit: a complete boot log with the last boot stage, first concrete failure/success signal, and triage classification.
- Blocker: requires a Windows machine plus the user's local game data.

## Next

[ ] **Verify triage and fix the first evidenced native blocker**
- Use the boot log and `tools/boot_triage.py`; do not speculate ahead.
- Classify into VM/PPU, VFS, HLE, GCM/RESC/RSX, SPURS/SPU, synchronization, audio, or input.
- Make the smallest faithful fix.
- Add a focused regression test or deterministic audit.
- Re-run relevant gates and checkpoint.

[ ] **Reach and verify first visible output**
- Confirm D3D12 initialization.
- Confirm at least one presented guest frame.
- Verify title/menu rendering stability.
- Record runtime evidence in `STATE.md` and `SESSION_LOG.md`.

[ ] **Verify vanilla controller gameplay**
- Confirm original controller mappings.
- Enter a mission.
- Verify simulation, placement/selection, pause, exit, save/load and clean shutdown.
- Fix only observed regressions.

[ ] **Resolve required SPU/audio behaviour**
- Determine whether the small active SPU workload is required for gameplay.
- Restore/fix audio/SPURS behaviour based on runtime evidence.
- Keep gameplay behaviour faithful; no approximate host-side replacement for core logic.

## Later

[ ] Native absolute mouse/world-pointer injection.
[ ] Configurable keyboard bindings beyond compatibility-mode mapping.
[ ] Safe aspect-ratio/display improvements.
[ ] High-refresh/frame-pacing work after simulation timing is proven.
[ ] Packaging/user-facing validation polish.
[ ] Network-service work only if required for local gameplay goals.

## Completed in the latest continuity cycle

[x] Restore advanced recovered source tree and make GitHub canonical.
[x] Harden exact ps3recomp pin/bootstrap/dependency handling.
[x] Add strict title/version/hash gates.
[x] Add deterministic analysis-baseline gate.
[x] Add PPU/SPU/HLE completeness gates.
[x] Add Windows real-lifter compile/link CI gate.
[x] Add native crash, hang and boot-summary diagnostics.
[x] Establish `STATE.md`, `WORK_QUEUE.md` and `PROJECT_PLAN.md` as the durable handoff structure.
[x] Add deterministic saved-boot-log subsystem triage with conservative `unknown` handling for generic crash/watchdog symptoms.\n[x] Integrate that triage directly into `comet_port.py run` so every native boot log automatically records subsystem/rationale.
