# NEXT

## Immediate

1. **Execute M1 against the actual NPEB00142 v1.00 files.**
   - Bootstrap the pinned toolkit with `tools/bootstrap_ps3recomp.ps1`.
   - Run `tools/m1_analyze.py` with the decrypted EBOOT ELF and, preferably, `PARAM.SFO`.
   - Compare against the established baseline: ~3,409 PPU functions; 171 imports / 18 libraries; 24 `cellSpurs` imports; 2 embedded SPU ELFs.
   - Inspect and commit only sanitized generated metadata/reports; never commit the EBOOT or extracted SPU binaries.

2. **Perform M2 first lift/build/run.**
   - Use `tools/m2_lift_build.ps1`.
   - Ensure the lift uses `--hle-stubs` and the generated HLE NID table.
   - Build with clang-cl + Ninja.
   - Capture the first boot log and identify the first real blocker.

3. **Checkpoint immediately after the first runtime result.**
   - Update `CONTINUITY.md` with the actual blocker.
   - Add the sanitized boot finding to `SESSION_LOG.md`.
   - Put the exact next fix at the top of this file.

## Blocker triage order after first boot

Unless runtime evidence establishes a different dependency order:

1. process/thread/memory/runtime primitives;
2. filesystem/VFS mapping;
3. GCM/Resc/graphics;
4. controller input;
5. remaining sysutil services;
6. audio/SPURS middleware.

Do not begin mouse/keyboard or PC polish until vanilla controller gameplay is stable.
