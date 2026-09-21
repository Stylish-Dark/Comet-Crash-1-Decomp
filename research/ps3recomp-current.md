# ps3recomp working baseline — 2026-09-22

## Pinned revision

Repository: https://github.com/sp00nznet/ps3recomp

Pinned commit:
`e2815326c58d3530936166982672cb09acdef4f9`

Commit title: **Generalize the lbp/ port out of the toolkit repo**

Why this revision matters for Comet Crash: it moves the reusable lifted-SPU arrangement into `templates/project/CMakeLists.txt`, including per-image lifted SPU sources, the musttail dispatch fast path and optional generated registration tables. Comet Crash already shows 24 `cellSpurs` imports and two embedded SPU ELFs, so using the post-generalization template avoids immediately carrying an obsolete pre-SPU starter arrangement.

## Current PPU pipeline

```text
python tools/ppu_loader.py game/EBOOT.ELF -o out/
python tools/ppu_lifter.py game/EBOOT.ELF --functions out/EBOOT.functions.json --hle-stubs out/EBOOT.imports.json --output recompiled/
python tools/gen_hle_nids.py --all --out recompiled/ppu_hle_nids.cpp
cmake -B build -G Ninja -DPS3RECOMP_DIR=/path/to/ps3recomp
cmake --build build
```

`--hle-stubs` is essential for a retail title. Without it, firmware import trampolines are lifted literally and indirect calls can land on the raw first instruction word (`0x39800000`) instead of the runtime HLE bridge.

The HLE NID registration table is the second half of the import bridge and must also be present.

## Windows requirements

Use clang-cl rather than MSVC `cl`. The runtime/lifted code uses compiler features such as `__atomic_*`, `__int128` and `__builtin_bswap*`.

Large generated PPU translation units require `/bigobj`; the current official template handles this.

## Current SPU tooling

- `tools/extract_spu_images.py <ppu_elf> --output <dir>`
- `tools/find_spu_functions.py <spu.elf> --out <functions.json>`
- `tools/spu_lifter.py`
- `tools/build_spu_workloads.py --images ... --lifted ... --out ...`

`build_spu_workloads.py` fingerprints guest SPU images and generates registrations for the runtime workload dispatcher. Extra function entries can be supplied for interrupt-vector cases that auto-function discovery cannot reach.

## Boot diagnostics worth recognizing

- `[ppu] unresolved indirect call -> 0x39800000`: check both `--hle-stubs` and the HLE NID table.
- `[HOTREAD] spinning on ...`: identify the writer that should advance the polled guest value.
- `[HLE] UNIMPLEMENTED`: inspect the NID/module and add/repair the runtime bridge rather than blindly returning success.

## Upstream references

- Getting started: https://github.com/sp00nznet/ps3recomp/blob/master/docs/GETTING_STARTED.md
- Project template: https://github.com/sp00nznet/ps3recomp/tree/master/templates/project
- Pinned commit: https://github.com/sp00nznet/ps3recomp/commit/e2815326c58d3530936166982672cb09acdef4f9
