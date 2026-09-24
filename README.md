# Comet Crash 1 PC Port

Experimental Windows-native static recompilation project for **Comet Crash** (PS3, NPEB00142 v1.00), built around `ps3recomp`.

This repository contains only original port tooling/code and reverse-engineering metadata. **It does not contain Comet Crash binaries, assets, PKGs, decrypted EBOOTs, or Sony SDK material.** Supply your own legally obtained game files locally.

## Current state

- Target binary fingerprint and SFO validation are pinned.
- Known PPU baseline: 3,409 unique functions, 171 firmware imports across 18 libraries.
- Both embedded SPU images are accounted for. The large image is MultiStream MP3 middleware; the small image is active but still unidentified.
- Full PPU + both embedded SPU lifts have completed successfully; reachable unsupported-instruction gates are clean after two Comet-specific VMX fixes.
- Controller polling/decoding is mapped and the Windows runner now overlays mouse input on the original `cellPadGetData` path while preserving controller support.
- F1 opens a host-rendered in-game Graphics & Input panel with resolution, windowed/borderless, VSync, mouse enable/sensitivity, and left/right-stick selection. Settings persist beside the EXE.
- Reproducible decrypt -> analyse -> PPU/SPU lift -> Windows build -> run commands are implemented.
- A Windows runtime boot/playability test is still required before this can be called playable; see `docs/STATUS.md`.

## Windows quick start

From a normal Command Prompt, run one command against your extracted game folder:

```bat
scripts\build_and_run.cmd "D:\Games\Comet Crash"
```

For a deliberately bounded first-boot attempt, a second argument applies a timeout to the native-run phase while preserving diagnostics:

```bat
scripts\build_and_run.cmd "D:\Games\Comet Crash" 60
```

The script enters the Visual Studio x64 developer environment automatically, clones/verifies the pinned `ps3recomp`, decrypts this title's FREE-NPDRM EBOOT, validates NPEB00142, analyses/lifts PPU + SPU code, builds the native runner with clang-cl, and launches it. It accepts either a folder containing `PARAM.SFO` + `USRDIR`, or its parent containing `PS3_GAME`.

Prerequisites: Visual Studio 2022/Build Tools with Desktop C++, Clang tools and a Windows SDK; Git; Python 3; CMake. Ninja is installed automatically if missing.

While running, press **F1** for the Graphics & Input panel. Resolution changes currently apply on the next launch; borderless/windowed and VSync are live. Mouse compatibility mode maps movement onto the selected PS3 analogue stick and maps LMB/RMB/MMB to Cross/Circle/R1 while retaining physical-controller input.

Each launch writes a timestamped `logs\boot-*.txt` plus `logs\boot-*.summary.json`. Preserve both if the first native run stops, crashes, times out or is interrupted. The structured summary records the last stage, first frame, first symptom, best subsystem-specific triage signal, conservative subsystem classification, outcome, exit/timeout/interruption state and the exact build provenance. Normal runs refuse stale/swapped EXEs whose hash/source/toolchain provenance does not match. Before sharing or debugging a result, run `python tools/verify_boot_bundle.py <boot.summary.json>`; `python tools/boot_triage.py <boot-log>` remains available for independent re-analysis.

See `docs/reversing/` for binary findings and `docs/superpowers/specs/` for the port architecture.

## Project handoff

For continued development, start with **`STATE.md`**. It is the authoritative handoff document. `WORK_QUEUE.md` contains bounded next work units, `PROJECT_PLAN.md` contains the high-level architecture/phases, and `SESSION_LOG.md` records completed work cycles.
