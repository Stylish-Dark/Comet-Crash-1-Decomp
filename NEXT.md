# NEXT

## Immediate

1. Add a repo-owned M1 analysis driver that:
   - validates a user-supplied decrypted PS3 ELF;
   - runs the pinned `ppu_loader.py`;
   - generates a named import list;
   - extracts embedded SPU ELFs into ignored local storage;
   - emits a non-proprietary JSON/Markdown compatibility report.

2. Add a Windows bootstrap/build script that:
   - obtains or validates the pinned `ps3recomp` revision;
   - runs the PPU lift with `--hle-stubs`;
   - configures CMake/Ninja with clang-cl;
   - never copies the user's EBOOT or game data into tracked paths.

3. Run M1 against NPEB00142 v1.00 and compare with the established baseline:
   - ~3,409 PPU functions;
   - 171 imports / 18 libraries;
   - 24 `cellSpurs` imports;
   - 2 embedded SPU ELFs.

## Then

4. Commit the regenerated compatibility metadata.
5. Perform the first PPU lift.
6. Build and launch the native runner with verbose diagnostics.
7. Record the **first actual blocker** rather than guessing which subsystem will fail.
8. Fix blockers in this order unless runtime evidence says otherwise:
   runtime primitives → filesystem/VFS → GCM/Resc graphics → controller → sysutil → audio/SPURS.
