# Exact-title model loader evidence capture

The repository **does not contain** the game's executable or the original
instruction stream at `0x00105308..0x0010B538`. Existing model geometry
notes establish staging strides and final vertex/index allocations, but
not the original signed index conversion, triple equality comparison or
triangulation order. Standard OBJ parser code in PR #31 is not evidence of
what the shipped loader did.

To capture the missing evidence from the **user's own decrypted
NPEB00142 v1.00 PPU ELF** (locally, no game data is uploaded):

```sh
python tools/decomp_model_disasm.py /path/to/EBOOT.ELF --start 0x00105308 --end 0x0010B538 --output generated/model-loader.txt
```

The tool validates a big-endian ELF64 header, maps virtual addresses
through a file-backed PT_LOAD, requires a contiguous aligned range and
full Capstone instruction decoding, and emits address-labelled instructions.
It refuses truncated/unmapped ranges instead of silently fabricating them.
For smaller reverse-engineering windows, override `--start` and `--end`;
`--format json` produces machine-readable instruction rows.

## Questions to answer from actual instructions

1. Locate the parser's `f` case and trace every conversion of parsed
   position, UV and normal indices. Record exact compare/branch operations,
   signedness and source-array count at each lookup.
2. Follow writes to the 12-byte staging vector; determine whether those
   fields are raw OBJ indices or already normalized source-array offsets.
3. Follow the staging-to-final index collapse. Identify the three equality
   comparisons, whether missing components have sentinel values, and whether
   source triples or packed vertex floats determine identity.
4. Trace how triangles are constructed from 3+ corner polygons and how
   material changes delimit 0x78-byte submesh draw ranges.
5. Cross-check against the original executable/oracle using representative
   faces (`1/1/1`, `-1/-1/-1`, `1//2`, repeated vertices, quads,
   out-of-range indices). Only promote a claim when the branch and write
   sites agree.

Do **not** put decrypted ELF content, large instruction dumps or game assets
in git. The `generated/` directory is for local capture artifacts.

This document is a reproducible recovery protocol, **not** a declaration
that the unresolved behaviour has been recovered.

The 2026-10-08 local capture produced additional instruction-supported findings in [model-loader-runtime-evidence.md](model-loader-runtime-evidence.md); the remaining questions above are still open.
