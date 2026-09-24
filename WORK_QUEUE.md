# WORK QUEUE

Work units are intentionally bounded so one worker can investigate, integrate, document and commit without approaching the execution limit.

## Current

[ ] **Capture first native Windows boot evidence**
- Input: supported NPEB00142 v1.00 extracted game directory.
- Run `scripts\build_and_run.cmd "<game folder>"`. If a bounded attempt is preferable, pass a native-run timeout as the second argument, e.g. `scripts\build_and_run.cmd "<game folder>" 60`.
- Preserve both `logs\boot-*.txt` and the adjacent `logs\boot-*.summary.json`.
- Confirm the summary contains build provenance, `boot_outcome`, first-frame state, `first_signal`, `triage_signal`, `suspected_subsystem`, `triage_rationale`, timeout/interruption state and exit code.
- Run `python tools/verify_boot_bundle.py <boot.summary.json>`; do not debug from an artifact that fails its log/provenance integrity checks.
- Success for this work unit: a self-verified, provenance-bound boot artifact that identifies how far the native title actually reached and the first concrete runtime blocker, if any.
- Blocker: requires a Windows machine plus the user's local game data.

## Next

[ ] **Verify triage and fix the first evidenced native blocker**
- Use the `.summary.json` plus the full text log; `tools/boot_triage.py` remains available for independent re-analysis. Do not speculate ahead.
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
[x] Add deterministic saved-boot-log subsystem triage with conservative `unknown` handling for generic crash/watchdog symptoms.
[x] Integrate triage directly into `comet_port.py run` and prefer later subsystem-specific evidence over an earlier generic watchdog.
[x] Preserve boot diagnostics across optional timeout and Ctrl+C; bound timeout termination with kill escalation.
[x] Emit structured `.summary.json` boot outcomes and embed exact build provenance.
[x] Enrich native crash/watchdog evidence with stage, frames, HLE breadcrumb and guest-VM AV location.
[x] Run the full 118-test suite on Linux and Windows CI; verify real pinned PPU lifter staging and clang-cl/Ninja native scaffold link.
[x] Fix Windows host-encoding drift in generated PPU auditing and prevent footer metadata from being reparsed as runtime signals.
[x] Enforce source/toolchain/HLE/native-EXE provenance before launch, with only an explicit recorded diagnostic override.
[x] Add self-verifying boot bundles that hash the finalized text log and re-derive the saved triage/outcome fields.
[x] Add exact HRESULT diagnostics for nine previously generic/silent D3D12 initialization exits.
[x] Run the expanded **131/131** Linux/Windows gate, including Windows repository safety, real pinned-lifter staging, native link, and linked-EXE provenance round-trip.
