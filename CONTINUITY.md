# CONTINUITY

## Ultimate objective

Produce a Windows-native port of **Comet Crash 1** from the user's legally obtained PS3 copy, preserving original controller gameplay first and then adding first-class mouse/keyboard controls and safe PC display/high-refresh improvements.

Target title: **NPEB00142**, user's **v1.00** copy.

## Current state

The repository was empty at the start of the 2026-09-22 work session. Prior structured design material was recovered and has now been made canonical here.

The project is at the boundary between **M1 reproducible analysis** and **M2 first native boot attempt**. There is **no recorded successful native build, visible output, or playable port yet**.

Current toolkit baseline is pinned to:

- upstream: `sp00nznet/ps3recomp`
- commit: `e2815326c58d3530936166982672cb09acdef4f9`
- commit date: 2026-09-21
- reason: this revision moves the title-agnostic lifted-SPU build arrangement into `templates/project`, which is directly relevant because Comet Crash imports `cellSpurs` and contains embedded SPU ELFs.

## Established facts

Previous analysis of the user's NPEB00142 v1.00 executable established:

- valid decrypted big-endian PPC64 PS3 ELF;
- approximately **3,409 PPU functions** detected;
- **171 firmware imports across 18 libraries**;
- **24 `cellSpurs` imports**;
- **two embedded SPU ELF programs**;
- the larger embedded SPU contains Sony MultiStream/MP3 identifiers and is strongly indicative of audio middleware rather than core gameplay simulation;
- the executable uses **`cellPad`**, giving the PC adaptation layer a contained interception point for controller compatibility and later keyboard/mouse work.

The current `ps3recomp` pipeline uses:

1. `tools/ppu_loader.py` for image / OPD / TOC / firmware-import analysis;
2. `tools/ppu_lifter.py ... --hle-stubs ...` for PPU lifting;
3. `tools/gen_hle_nids.py --all` for the runtime NID→HLE registration table;
4. the project template + CMake/Ninja to build and boot the lifted title.

On Windows, use **clang-cl**, not MSVC `cl`, and retain `/bigobj` for large lifted translation units.

## Working hypotheses

- The large embedded SPU is Sony MultiStream/MP3 audio middleware.
- Audio/SPURS may be deferrable during first-light work if it blocks rendering, but only behind an explicit development-only bypass.
- The smaller embedded SPU may be unrelated to gameplay, but its role is **not established**.
- Static recompilation is technically plausible for this title, but successful boot still depends on actual runtime/HLE coverage and game-specific behavior.

## Completed

- Recovered the prior project objective and architecture.
- Recovered the prior executable-analysis findings listed above.
- Re-checked the current `ps3recomp` toolchain and project template.
- Chosen an exact upstream commit to prevent silent toolchain drift.
- Initialized this repository as the canonical continuity/checkpoint system.

## Approaches attempted / considered

- Static recompilation via `ps3recomp`: retained.
- Full emulator-based delivery: not the target; RPCS3 remains a behavioral comparison baseline only.
- Reimplementing gameplay logic in host code merely to get something running: rejected.
- Mouse/keyboard-first work: rejected until vanilla controller gameplay is stable.
- Redistributing original game binaries/assets: prohibited.

## Unresolved questions

- Exact hashes of the user's v1.00 decrypted executable and source tree have not yet been captured into a non-proprietary manifest.
- The precise 171-import list and per-library counts need to be regenerated and stored in the repo.
- The role of the smaller embedded SPU ELF is unknown.
- It is not yet known which embedded SPU images are actually submitted to SPURS, or at what point in boot.
- The first unsupported HLE/runtime failure is unknown because no current native boot attempt has been captured.
- The repo does not yet contain a reproducible M1 automation script.

## Current bottleneck

The project needs a reproducible, repo-owned M1 pipeline that accepts the user's local decrypted `EBOOT.ELF`, runs the pinned `ps3recomp` analysis tools, captures import/function/SPU metadata without committing proprietary binary data, and prepares the exact inputs for the first PPU lift.

## Exact next useful action

Create and commit the M1 automation + compatibility-report tooling, then run it against the user's NPEB00142 v1.00 decrypted executable. After the report matches the established ~3,409 functions / 171 imports / two SPU images baseline, perform the first PPU lift with `--hle-stubs` and build the port scaffold.

## Important supporting files

- `docs/DESIGN.md` — architecture, milestones, legal boundary, testing strategy.
- `research/ps3recomp-current.md` — current upstream commands, compiler requirements, SPU tooling, pinned revision.
- `DECISIONS.md` — settled project decisions and rejected approaches.
- `NEXT.md` — current concrete work queue.
- `SESSION_LOG.md` — chronological work-session record.
