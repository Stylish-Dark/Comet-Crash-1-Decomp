# Comet Crash 1 PC Port

## Completion map — 2026-10-22

See [PROJECT_PLAN.md](PROJECT_PLAN.md) for what is done, partial and missing,
implementation order and runnable acceptance criteria. Current code baseline is
PR #63. A preview runs; the original player loop and first original mission are
not yet accepted. Next: P1 full controller, connected toward G1, then G2 original
mission. Earlier preview percentages are historical and withdrawn.

**Project contract, corrected 2026-10-20:** faithfully recover the original
game and adapt it to PC with mouse-capable controls. Current focus is original
code recovery, not expansion of replacement preview behavior. Earlier
subjective progress percentages remain withdrawn. The full-port planning baseline
is about 25% (plausible 15–35%); see [assessment](docs/decomp/completion-assessment.md). See [AGENTS.md](AGENTS.md) and [fidelity status](docs/decomp/fidelity-status.md).


Native PC source-recovery project for **Comet Crash** (PS3, NPEB00142 v1.00).

The project is now explicitly a **decompilation/native rewrite**, not a static-recompilation runtime as the final product. The objective is to recover the game's own logic into readable C/C++, reconstruct its data structures and subsystem boundaries, replace PS3-specific interfaces with native PC implementations, and compile the result as an ordinary Windows game.

The previous `ps3recomp` work is retained because it provides valuable exact machine-code lifting, function boundaries, dynamic traces and a behavioural oracle. It is no longer the target architecture.

This repository contains original tooling/code and reverse-engineering metadata only. **It does not contain Comet Crash binaries, assets, PKGs, decrypted EBOOTs, or Sony SDK material.** The user's legally obtained NPEB00142 v1.00 data remains private input.

## Current state

- Reference title is pinned: 3,409 original PPU functions, 171 firmware imports across 18 libraries, two embedded SPU images.
- The old static-recomp track reached a native x86-64 Windows process, D3D12 initialization, guest-frame presentation and real asset/draw activity. That runtime proved useful for tracing but also confirmed that carrying PS3 execution semantics forward is the wrong end architecture.
- A dedicated source-recovery tree now lives in `decomp/`.
- `tools/decomp_source_refs.py` maps stripped PPU functions to surviving source/string anchors through the module TOC.
- The first recovered game-specific function is `0x000D5D5C`, a level-map path/load wrapper translated into semantic C++ in `decomp/recovered/level_map_000D5D5C.cpp`.
- `0x000E5A34` is now firmly tied to original `arenaGraphics.cpp` lines 1034-1188 via seven surviving source-line strings.

## Development direction

Work proceeds subsystem by subsystem:

1. recover function identity, control flow, data structures and file formats from the exact PS3 executable;
2. write clean semantic C/C++ equivalents under `decomp/`;
3. replace PS3 graphics/audio/input/filesystem/threading boundaries with native PC modules;
4. differential-test recovered behaviour against the original title/static-recomp oracle;
5. retire compatibility-runtime code as each subsystem becomes native.

For project continuity, read **`STATE.md`** first, then `WORK_QUEUE.md`, `PROJECT_PLAN.md`, `DECISIONS.md`, and `SESSION_LOG.md`.
