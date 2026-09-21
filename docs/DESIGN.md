# Comet Crash 1 PC Port Design

## Objective

Create a Windows-native port of the user's legally owned PS3 copy of **Comet Crash** (NPEB00142 v1.00) using `ps3recomp` static recompilation.

The first working build must preserve original controller gameplay before PC-specific enhancements are introduced.

## Architecture

1. **Recompiled guest code** — lifted PPU code and only the SPU workloads proven necessary.
2. **PS3 compatibility runtime** — `ps3recomp` HLE/runtime plus narrowly scoped title-specific shims.
3. **PC adaptation layer** — filesystem mapping, display behavior, controller, keyboard, mouse and configuration.
4. **Build/installer tooling** — accepts the user's own game files locally and never commits proprietary data.

## Known title facts

Prior analysis of NPEB00142 v1.00 found:

- valid decrypted big-endian PPC64 ELF;
- ~3,409 PPU functions;
- 171 firmware imports across 18 libraries;
- 24 `cellSpurs` imports;
- two embedded SPU ELF programs;
- larger SPU strongly indicative of Sony MultiStream/MP3 audio middleware;
- `cellPad` used for controller input.

## Milestones

### M1 — Reproducible analysis

A fresh checkout plus user-supplied game files can reproduce function/import/SPU analysis without manual binary editing.

Outputs should include:
- executable fingerprint/manifest;
- loader/function/import summaries;
- named import report;
- embedded-SPU inventory;
- machine-readable compatibility report.

### M2 — First native process / boot attempt

- Lift PPU code with the pinned toolkit and `--hle-stubs`.
- Build against the official project scaffold.
- Launch with verbose diagnostics.
- Identify the first real unsupported import/runtime failure.

Success means the host process reaches the recompiled entry path, even if boot then stops.

### M3 — Reach visible output

Prioritize:
1. process/thread/memory/runtime primitives;
2. filesystem and game-data path mapping;
3. GCM/Resc graphics;
4. controller input;
5. remaining sysutil services;
6. audio/SPURS middleware.

Audio may only be bypassed behind an explicit development flag, and only to expose later boot/render blockers.

### M4 — Playable vanilla port

Confirm:
- original controller mappings;
- mission loading;
- gameplay simulation;
- pausing;
- saving/loading;
- clean exit;
- behavior comparable with PS3/RPCS3 baseline.

### M5 — Native mouse and keyboard

Two stages:

1. compatibility mode: host mouse/keyboard drive the existing controller-facing abstractions;
2. native pointer mode: hook the game's actual cursor/selection state so mouse movement is direct rather than analogue-stick emulation where technically feasible.

### M6 — PC polish

Only after vanilla stability:
- windowed/borderless/fullscreen;
- configurable resolution;
- safe aspect-ratio handling;
- frame-pacing/high-refresh investigation;
- configurable bindings;
- user-facing validation/diagnostics.

## Audio/SPU strategy

Do not infer necessity from mere presence in the executable. Determine dynamically which SPU images are submitted through SPURS and when.

The large embedded image is currently a **working hypothesis** for MultiStream/MP3 audio middleware, not a proved gameplay dependency.

## Testing

Every milestone gets a repeatable smoke test and captured runtime log. Runtime/HLE fixes should receive focused tests where practical. Gameplay behavior is compared against a known-good PS3/RPCS3 baseline using equivalent state/mission where possible.

## Initial non-goals

- Comet Crash 2.
- network-service restoration beyond what local play requires.
- asset replacement/remastering.
- distributing a standalone package containing copyrighted game data.
- major UI/visual redesign before original gameplay is stable.
