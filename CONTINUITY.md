# CONTINUITY

## Ultimate objective

Produce a Windows-native static-recompilation port of **Comet Crash 1** from the user's legally obtained PS3 copy (**NPEB00142 v1.00**), preserve original controller gameplay, then add first-class PC input/display improvements without redistributing proprietary game data.

## Current state

This recovered checkpoint is materially ahead of the temporary scaffold that replaced the GitHub repository. It is the authoritative engineering state to preserve and restore.

Current project state:

- deterministic local FREE-NPDRM EBOOT decryption is implemented;
- title/SFO/ELF validation is implemented and fingerprints are pinned;
- upstream analysis independently confirms **3,409 unique PPU functions**, **171 firmware imports across 18 libraries**, **24 `cellSpurs` imports**, and **2 embedded SPU ELFs**;
- full PPU lift and both embedded SPU lifts have completed;
- reachable unsupported-instruction gates are clean after the two explicit VMX fixes (`vsrab`, `vsrb`);
- Windows native runner, D3D12 integration patches, VFS/HDD mappings, RESC/GCM fixes, SPURS urgent-command handling, and Comet-specific HLE overrides are implemented;
- controller input is preserved while mouse compatibility-mode input overlays the exact `cellPadGetData` path;
- F1 host Graphics & Input overlay and persistent settings are implemented;
- the one-command Windows pipeline is `scripts/build_and_run.cmd`;
- first-boot diagnostics now produce a durable boot log and fine-grained native initialization stage markers;
- current local regression gate: **61/61 tests passing**, plus `compileall` and repository-safety checks.

**Not yet established:** successful native Windows boot, visible title/menu output, or playable missions. This environment cannot execute the Windows/D3D12 build.

## Toolchain pin

`ps3recomp` is pinned by `config/ps3recomp.lock` to:

`d3ed1a5c946a9c5370b51631e13371a1adf70396`

Do not silently move this pin. Runtime source patchers deliberately fail on upstream drift.

## Established facts

- Reference rebuilt ELF: 2,660,448 bytes, SHA-256 `3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
- Legacy accepted reconstruction: `cf349416cd7f13cf77ac8252ea41c3676d1674496533d2904cf1acd49f516b32`.
- 3,473 OPD descriptors produce 3,409 unique PPU functions.
- 171 firmware imports across 18 libraries.
- 24 `cellSpurs` imports.
- Embedded SPU sizes: 2,952 bytes and 95,264 bytes.
- Large SPU is Sony MultiStream MP3 middleware.
- Small SPU is actively consumed but still unidentified.
- `cellPadGetData` and the downstream pad decoder are mapped.
- Full PPU lift emitted 3,744 functions after boundary recovery/tail wrappers.
- Small SPU: 28 reachable functions, zero reachable unsupported instructions.
- MultiStream SPU: 1,099 functions reachable from entry `0x3050`; 446 unsupported `.word` markers exist only outside the reachable set.
- All 171 Comet firmware imports are required to be covered by pinned-runtime HLE or explicit port overrides before the native build proceeds.

## Settled engineering decisions

- Static recompilation via `ps3recomp` is the implementation path.
- Preserve vanilla/controller behavior before native pointer injection or timing/render experiments.
- Do not fake core gameplay behavior with approximate host logic.
- Optional PS3 online/media services must fail explicitly or operate in a deliberately offline-safe manner rather than returning fabricated success.
- SPU dependencies are handled from observed dispatch behavior, not assumptions based on embedded binaries alone.
- Proprietary game files stay local and ignored.

## Important completed work

- `tools/decrypt_comet_self.py` — local FREE-NPDRM SELF reconstruction.
- `tools/ps3_probe.py` — PPU/import/SPU/callsite probing.
- `tools/comet_port.py` — decrypt → validate → analyse → lift → build → run pipeline.
- `tools/patch_ppu_lift.py` — explicit Comet VMX fixes.
- `tools/audit_spu_lift.py` — reachable SPU unsupported-instruction gate.
- `tools/audit_hle_coverage.py` — exact 171-import coverage gate; port overrides count only when their NID is actually registered, not merely declared as a constant.
- `tools/patch_ps3recomp_{host,spurs,vfs,resc,gcm}.py` — deterministic pinned-runtime patches.
- `port/comet_compat.*` — title-specific compatibility HLE.
- `port/comet_host.*` — mouse/controller coexistence, overlay, host controls.
- `port/comet_settings.*` — persistent settings.
- `scripts/build_and_run.cmd` — one-command Windows pipeline.
- Boot diagnostics now write `logs/boot-YYYYMMDD-HHMMSS.txt` by default and include stage markers through PPU entry plus the first presented guest frame.

## Working hypotheses / unresolved questions

- Small SPU program remains unidentified despite being active.
- Direct absolute mouse pointer injection remains unresolved; current mouse input intentionally uses analogue compatibility mode.
- Native runtime may expose further HLE, RSX, SPURS, synchronization or VFS defects that cannot be proved without a Windows run.
- Frame-rate unlocking is intentionally deferred until gameplay timing is proven stable.

## Current bottleneck

**The first real Windows native boot must be executed against the user's extracted NPEB00142 v1.00 game.**

Static analysis and Linux-side regression work have reached diminishing returns. The next high-value evidence is the native boot log.

## Exact next useful action

On a Windows machine with Visual Studio 2022/Build Tools (Desktop C++, Clang tools, Windows SDK), Git, Python 3 and CMake:

```bat
scripts\build_and_run.cmd "D:\Games\Comet Crash"
```

The script should now create a boot log under `logs\boot-YYYYMMDD-HHMMSS.txt` automatically.

After the run:

1. preserve the complete boot log;
2. identify the **last `[boot-stage]` reached**;
3. capture the first `[HLE] UNIMPLEMENTED`, unresolved indirect call, HOTREAD/spin, access violation, RSX error, SPURS failure, or other terminal symptom;
4. update this file and `NEXT.md` with the actual blocker before changing code;
5. fix one evidenced blocker at a time and checkpoint it.

## Important supporting files

- `docs/STATUS.md` — concise engineering status.
- `docs/reference/known-analysis.json` — verified binary-analysis baseline.
- `docs/reversing/input.md` — controller/input reversing.
- `docs/reversing/spu.md` — SPU findings.
- `docs/superpowers/specs/2026-09-18-comet-crash-pc-port-design.md` — architecture/specification.
- `config/ps3recomp.lock` — exact upstream pin.
- `NEXT.md` — immediate work queue.
- `DECISIONS.md` — durable decisions/rejected approaches.
- `SESSION_LOG.md` — chronological checkpoints.
