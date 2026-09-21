# Comet Crash 1 PC Port Design

## Objective

Create a Windows-native port of the user's legally owned PS3 copy of **Comet Crash** (NPEB00142) using the current `ps3recomp` static-recompilation ecosystem. The port must preserve original controller gameplay while adding native mouse and keyboard controls. Proprietary game data must never be committed or redistributed.

## Known source-title facts

Current analysis of the user's v1.00 NPEB00142 copy found:

- A valid decrypted big-endian PPC64 PS3 ELF.
- Approximately 3,409 detected PPU functions.
- 171 firmware imports across 18 PS3 libraries.
- 24 `cellSpurs` imports.
- Two embedded SPU ELF programs.
- The large embedded SPU program contains Sony MultiStream/MP3 identifiers and appears to be audio middleware rather than Comet Crash gameplay simulation.
- The executable uses `cellPad` for controller input, giving the PC port a contained interception point for input adaptation.

These facts make a static-recompilation approach technically plausible but do not guarantee that the unmodified game will boot without game-specific runtime/HLE work.

## Architecture

The project will be split into four independently testable layers:

1. **Recompiled guest code** — generated PPU code (and any required lifted SPU code) produced from a user-supplied decrypted game executable.
2. **PS3 compatibility runtime** — `ps3recomp` runtime/HLE support for imported PS3 services used by Comet Crash. Game-specific shims live separately from upstream runtime code where possible.
3. **PC adaptation layer** — Windows-side display, filesystem path mapping, controller, keyboard, mouse, and configuration behavior.
4. **Installer/build tooling** — scripts that accept a user's legally obtained game files locally, validate the expected title/version, generate required outputs, and build the Windows executable without committing proprietary data.

The first successful build must preserve original game behavior before PC-specific enhancements are introduced.

## Milestones

### M1 — Reproducible analysis/build input

- Validate NPEB00142 v1.00 input.
- Re-run `ps3recomp` loader/function/import/SPU analysis from scripts checked into this repository.
- Generate build artifacts only into ignored directories.
- Produce a machine-readable compatibility report.

Success: a fresh checkout plus user-supplied game files can reproduce the same PPU/import/SPU analysis without manual binary editing.

### M2 — First native process / boot attempt

- Lift the PPU executable with current `ps3recomp` tooling.
- Wire the generated code into the Windows runtime.
- Build a native Windows executable.
- Run it with verbose logging and identify the first unsupported import/runtime failure.

Success: a native executable starts and reaches the recompiled entry point, even if it then fails on an unsupported subsystem.

### M3 — Reach visible game output

Prioritise boot blockers in this order:

1. process/thread/memory/runtime primitives;
2. filesystem and game-data path mapping;
3. GCM/Resc/graphics path;
4. controller input;
5. remaining sysutil services;
6. audio/SPURS middleware.

Audio may be temporarily stubbed or disabled if it blocks reaching visible output, but only behind an explicit development flag.

Success: title/menu/game output renders reliably on Windows.

### M4 — Playable vanilla port

- Restore required audio/SPURS behavior.
- Confirm original controller mappings.
- Confirm mission loading, gameplay simulation, pausing, saving/loading, and clean exit.
- Compare behavior against the PS3/RPCS3 baseline.

Success: the original game is playable end-to-end with controller input before PC control changes.

### M5 — Native mouse and keyboard

Two-stage implementation:

1. **Compatibility mode:** mouse/keyboard drive the existing controller-facing input abstractions so gameplay can be tested quickly.
2. **Native pointer mode:** identify the game's world/UI cursor state and map host mouse coordinates directly into the cursor/selection logic, avoiding analogue-stick emulation for pointer movement.

Controller support remains intact. Mouse and keyboard support is additive, not a replacement.

Success: menus and gameplay can be comfortably operated with mouse/keyboard, with direct pointer motion rather than stick-like acceleration where technically feasible.

### M6 — PC polish

Only after vanilla gameplay is stable:

- windowed/borderless/fullscreen modes;
- configurable resolution;
- high-refresh/frame-pacing investigation;
- safe aspect-ratio handling;
- configurable input bindings;
- user-facing diagnostics for invalid/missing game files.

Any frame-rate unlock must separate rendering cadence from gameplay simulation timing if the original game couples them.

## Input design

The port will expose a host input state independent of PS3 pad structures. Adapters will translate that state to:

- original `cellPad` semantics for compatibility/controller behavior;
- direct cursor/UI hooks for native mouse behavior once the relevant guest functions/state are identified.

This keeps PC input changes isolated from the static recompilation core and allows controller regression testing.

## Audio/SPU strategy

Do not assume all SPU code is required at first boot. Determine dynamically which embedded SPU images are actually submitted through SPURS and when. The identified MultiStream MP3 workload may initially be bypassed to reach rendering/gameplay, then restored via lifted SPU execution or a clean host-side replacement only if legally and technically appropriate.

Gameplay behavior must never be silently replaced with approximate host logic merely to make the executable run.

## Repository/legal boundary

Never commit or distribute:

- Comet Crash PKG/RAP/RIF files;
- EBOOT.BIN, decrypted ELF, or embedded copyrighted game binaries;
- extracted Comet Crash assets/data archives;
- Sony SDK headers/libraries/material not legally redistributable.

The repository may contain original port code, build scripts, generated metadata that does not embed copyrighted game content, patches expressed as addresses/signatures/logic, and documentation. Users supply their own original game data locally.

## Testing strategy

Every milestone gets a repeatable smoke test and captured runtime log. Important runtime/HLE fixes receive focused unit or conformance tests where the `ps3recomp` runtime supports them. Gameplay regressions are checked against a known-good PS3/RPCS3 baseline using the same mission/state where practical.

For mouse controls, testing must cover menu navigation, placement/selection precision, camera movement, cancellation/back actions, split/local multiplayer interactions where applicable, and coexistence with controller input.

## Initial non-goals

- Comet Crash 2.
- Multiplayer/network-service restoration beyond what is necessary for local play.
- Asset replacement/remastering.
- Distribution of a standalone build containing copyrighted game data.
- Large visual/UI redesigns before the original game is stably playable.
