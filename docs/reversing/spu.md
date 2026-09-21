# Embedded SPU workloads

The reference PPU ELF contains two embedded 32-bit big-endian SPU ELFs.

## Small image — active, role unknown

- PPU ELF file offset `0x001B4980`
- PPU virtual address `0x001C4980`
- size 2,952 bytes
- SHA-256 `946ae7cbd718d4404f1be8edfe967cd85052f5a4c19c472ee35acf102b17220f`
- `.note.spu_name`: `/home/kanee/svnwork/ps3-svn/svn/`
- GCC 4.1.1 / SDK 2.80 provenance

This image is not dead data. A pointer to `0x001C4980` is stored at PPU `0x0023A1F8`. PPU function `0x0002C2FC` loads that pointer at `0x0002C354`, derives the image extent and passes it into the downstream ELF parser at `0x000BD150`. Treat it as potentially game-specific until proven otherwise.

## Large image — MultiStream MP3

- PPU ELF file offset `0x00207580`
- PPU virtual address `0x00217580`
- size 95,264 bytes
- SHA-256 `9a3889076b6c874ac191ae9d5f97bb26fdec2f5a0739ec7efe6e986fb2ed15bf`
- `.note.spu_name`: `spu/msngSPURS_MP3`

Strings identify this as Sony MultiStream MP3/audio middleware.

## Build implication

Both images are extracted and lifted with `ps3recomp/tools/build_spu_workloads.py`, then registered by content fingerprint. The project deliberately fails CMake configuration when the SPU registry/lifted images are absent rather than silently producing an incomplete port.
