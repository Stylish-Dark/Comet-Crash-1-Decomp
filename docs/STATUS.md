# Status

Current target: NPEB00142 v1.00.

## Verified binary facts

- Deterministic RPCS3-style rebuilt PPU ELF: 2,660,448 bytes, SHA-256 `3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
- 3,409 unique PPU functions from 3,473 OPD descriptors.
- 171 firmware imports across 18 libraries.
- 24 `cellSpurs` imports.
- Two embedded SPU programs (2,952 and 95,264 bytes). Large = MultiStream MP3 middleware; small = actively consumed but unidentified.
- `cellPadGetData` path and downstream pad decoder are mapped.

## Engineering state

- Reproducible local FREE-NPDRM decrypt/validation/analyse/lift/build/run pipeline exists.
- `ps3recomp` pin: `d3ed1a5c946a9c5370b51631e13371a1adf70396`.
- Full PPU lift completed: 3,744 emitted functions after boundary recovery/tail wrappers.
- Small SPU: 28 reachable functions, zero reachable unsupported instructions.
- MultiStream SPU: 1,099 reachable functions from entry `0x3050`; all 446 unsupported `.word` markers are outside the reachable set.
- Explicit VMX lift fixes cover the two real v0.12.1 PPU holes: `vsrab`, `vsrb`.
- Windows host includes persistent settings, F1 Graphics & Input overlay, live windowed/borderless, live VSync patch, mouse/controller coexistence, SPURS urgent-command support, VFS/HDD mappings, RESC/GCM fixes and explicit Comet HLE compatibility.
- Build refuses to continue unless all 171 Comet imports are covered.
- Recovered local regression gate: **61/61 tests passing**, plus compileall and repository-safety checks.
- First-boot diagnostics now mirror native stdout/stderr into `logs/boot-YYYYMMDD-HHMMSS.txt`, record ELF/runtime metadata, emit granular `[boot-stage]` markers, and mark the first presented guest frame.

**Not yet claimed:** successful native Windows boot/playability. The next engineering phase is a real Windows run and evidence-driven repair of the first runtime blocker.
