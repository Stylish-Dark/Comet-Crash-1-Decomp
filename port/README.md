# Port scaffold

This directory is the native runner/build scaffold for Comet Crash.

The initial `CMakeLists.txt`, `main.cpp`, `stubs.cpp` and `config.toml` are derived from the MIT-licensed `sp00nznet/ps3recomp` project template at:

`e2815326c58d3530936166982672cb09acdef4f9`

Local lifted PPU output belongs in `port/recompiled/` and is gitignored. The title-specific `stubs.cpp` should remain minimal until a captured boot failure proves an override is required.

Use `tools/m2_lift_build.ps1` after M1 analysis has generated loader metadata.
