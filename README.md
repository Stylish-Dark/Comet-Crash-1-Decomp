# Comet Crash 1 PC Port

Experimental Windows-native static recompilation project for **Comet Crash** (PS3, NPEB00142 v1.00), built around `ps3recomp`.

This repository contains only original port tooling/code and reverse-engineering metadata. **It does not contain Comet Crash binaries, assets, PKGs, decrypted EBOOTs, or Sony SDK material.** Supply your own legally obtained game files locally.

> **Recovery note (2026-09-22):** a materially more advanced project checkpoint was recovered after the GitHub branch had been reset/replaced. Read `CONTINUITY.md` and `RECOVERY.md` before using source files. The recovered advanced source snapshot is preserved persistently while the remote working tree is reconciled.

## Current engineering state

- Target binary fingerprint and SFO validation are pinned.
- Known PPU baseline: 3,409 unique functions, 171 firmware imports across 18 libraries.
- Both embedded SPU images are accounted for. The large image is MultiStream MP3 middleware; the small image is active but still unidentified.
- Full PPU + both embedded SPU lifts have completed successfully; reachable unsupported-instruction gates are clean after two Comet-specific VMX fixes.
- Controller polling/decoding is mapped and the Windows runner overlays mouse input on the original `cellPadGetData` path while preserving controller support.
- F1 host Graphics & Input overlay and persistent settings are implemented.
- SPURS urgent-command handling, VFS/HDD mappings, RESC/GCM fixes and explicit Comet HLE compatibility overrides are implemented.
- Reproducible decrypt → analyse → PPU/SPU lift → Windows build → run commands are implemented.
- First-boot diagnostics now write a timestamped native boot log with granular `[boot-stage]` markers.
- Current recovered regression gate: **61/61 tests passing**, plus compileall and repository-safety checks.
- A real Windows native boot/playability run is still required before the port can be called playable.

## Windows quick start

From the recovered advanced tree, run:

```bat
scripts\build_and_run.cmd "D:\Games\Comet Crash"
```

Prerequisites: Visual Studio 2022/Build Tools with Desktop C++, Clang tools and a Windows SDK; Git; Python 3; CMake.

Each launch writes `logs\boot-YYYYMMDD-HHMMSS.txt`. Preserve that file if the native run stops or crashes; it is the primary next-debugging artifact.
