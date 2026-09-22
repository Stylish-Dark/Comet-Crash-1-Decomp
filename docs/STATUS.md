# Status

Current target: NPEB00142 v1.00.

## Verified binary facts

- Deterministic RPCS3-style rebuilt PPU ELF: 2,660,448 bytes, SHA-256 `3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`. The earlier `cf3494...` reconstruction remains accepted because its executable/loadable content yields the same verified analysis.
- 3,409 unique PPU functions from 3,473 OPD descriptors.
- 171 firmware imports across 18 libraries.
- 24 `cellSpurs` imports.
- Two embedded SPU programs. The large one is MultiStream MP3 middleware; the small one is actively consumed but remains unidentified.
- `cellPadGetData` path and downstream pad decoder are mapped; see `docs/reversing/input.md`.

## Current engineering state

- Reproducible decrypt/validation/analyse/lift/build/run CLI is present. The original retail NPEB00142 EBOOT is NPDRM FREE-license and can be rebuilt locally without a RAP.
- `ps3recomp` is pinned to one upstream commit.
- PPU and SPU lifts are separate generated outputs and proprietary inputs are ignored.
- Windows runner uses the upstream D3D12/HLE runtime contract.
- The current workspace has reproduced the decrypted ELF directly from the user-owned retail EBOOT and has now run the upstream `ps3recomp` v0.12.1 SDK analysis pipeline against it. Upstream `ppu_loader.py` independently reports 3,473 OPD descriptors / 3,409 unique functions and 171 firmware imports across 18 libraries; upstream `extract_spu_images.py` independently extracts the same 2 SPU ELFs (2,952 and 95,264 bytes).

The repository remains pinned to upstream commit `d3ed1a5c946a9c5370b51631e13371a1adf70396`. That exact commit has green Linux/Windows/macOS CI but no downloadable CI artifact. For sandbox-only lift experiments, the published v0.12.1 Linux SDK bundle is used as a bootstrap because the sandbox cannot perform ordinary GitHub clones. Final reproducible builds should still use the pinned commit, and any lift difference must be reconciled before generated code is promoted.

## Lift and Windows host status

- First full lift completed: 3,744 PPU functions emitted after boundary recovery/tail wrappers.
- The small SPU lift has 28 reachable functions with zero unsupported instructions.
- The MultiStream MP3 SPU has 1,099 functions reachable from entry 0x3050; all 446 unsupported `.word` markers are outside that reachable set.
- The exact pinned PPU lifter still leaves the two known Comet VMX holes (`vsrab`, `vsrb`); `tools/patch_ppu_lift.py` patches them alias-safely and the lift command applies that automatically. A post-patch PPU completeness audit then fails the lift if any generated `/* TODO: ... */;` instruction remains.
- The Windows host layer has persistent settings, an F1 in-game Graphics & Input overlay, live windowed/borderless switching, a live VSync switch (via a deterministic ps3recomp D3D12 source patch), and mouse/controller coexistence through an exact `cellPadGetData` HLE override.
- Mouse movement currently uses compatibility-mode analogue injection. Direct absolute world/UI-pointer injection remains a later refinement after the native boot path is proven.
- `scripts/build_and_run.cmd` automates the entire user-owned-game -> native EXE pipeline on Windows.
- Comet-specific `cellSpursAddUrgentCommand` support is wired end-to-end: a real four-slot guest FIFO is consumed by the host job-chain walker before the normal command stream, preserving the title's direct-JOB and RESET_PC usage.
- A Comet-specific HLE compatibility layer covers verified gaps that would otherwise hit ps3recomp's unresolved-import success fallback: `_cellGcmFunc15`, `cellRescSetWaitFlip`, `sys_net_free_thread_context`, NP score/region calls, trophy abort, and PS3-only recording/video export/upload services. Optional online/media services fail explicitly or operate in offline-safe mode instead of returning fabricated success and leaving the game waiting for callbacks.
- Current verified regression gate: **118/118 tests passing**; `compileall` and repository-safety gates pass on GitHub Actions. The build now generates the pinned runtime HLE table and refuses to continue unless all 171 Comet imports are covered by runtime handlers or actually registered port overrides; declaring an NID constant without registering it no longer produces a false-green coverage result. Host/settings/compat and the Windows main translation unit also pass a whole-port syntax compile gate.
- Fresh-Windows preflight is hardened: VS environment detection uses `VSCMD_VER`; Ninja may come from PATH or the Python wheel and its concrete executable is passed to CMake; clang-cl is configured with no strict-aliasing assumptions and FP contraction disabled.
- CI now has a Windows scaffold compile/link job against the exact locked ps3recomp commit. It first runs the full Python regression/`compileall` suite on Windows, applies every Comet runtime patch, runs the real pinned PPU lifter on a non-proprietary one-instruction PPC fixture, applies the same compatibility patch/completeness audit as the Comet pipeline, and then compiles/links that generated output. The SPU side remains synthetic for now. Generated PPU audits are byte-safe across host code pages rather than assuming UTF-8.
- First-boot diagnostics are now durable: the run command mirrors combined native stdout/stderr into `logs/boot-YYYYMMDD-HHMMSS.txt` and emits an adjacent `.summary.json`. Triage preserves the first chronological symptom but prefers a later subsystem-specific signal when available; generic crash/watchdog symptoms remain `unknown`. Evidence includes first-frame state, outcome, timeout/interruption status, subsystem/rationale and exact build provenance. Native crashes additionally record boot stage, frame count, last HLE breadcrumb and whether an access-violation target lies inside guest VM memory. Ctrl+C and optional run timeouts preserve a final diagnostic footer; timeouts are strictly bounded by terminate/kill escalation.

GitHub Actions run `35748082438` is the current authoritative pre-boot gate: **118/118 tests passed**, `compileall` and repository safety passed, the same unit suite passed on Windows, the real pinned PPU lifter generated its fixture successfully, the Comet compatibility patch and byte-safe PPU audit accepted it, and clang-cl/Ninja linked `CometCrashPC.exe` successfully.

**Not yet claimed:** successful native Windows boot/playability. This Linux execution environment has no Windows SDK/D3D12 runtime, so the real EXE must still be built/run on Windows. The next engineering phase is first-boot logging and resolving any title-specific HLE/RSX/SPURS runtime failures until missions are playable.
