# Original model-loader evidence, 2026-10-08

Locally decoded the user-owned NPEB00142 v1.00 PPU ELF, SHA-256
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
Reproduce using `tools/decomp_model_disasm.py` and the local capture procedure in
`model-loader-evidence.md`. Original binaries and instruction dumps remain private.

## Diffuse texture suffix

In the MTL diffuse-texture branch around `0x0010A6A0`, the string-length check at
`0x0010A6E8..0x0010A6EC` skips rewriting for length <= 3. Otherwise, stores at
`0x0010A718`, `0x0010A740`, and `0x0010A764` replace the last three characters
with byte values 0x64, 0x64, 0x73 (`dds`). This explains why shipped MTL files
retain `.tif` names while the corresponding assets are DDS. Native
`replace_extension(".dds")` agrees on the shipped names, but does not reproduce
the original last-three-character mutation for arbitrary unusual filenames.

Rooted author paths occur in the shipped Tesla, base-shell and mine materials.
Native loading uses their basename in the material directory. This is a portable
policy, not a newly proven legacy path-helper behavior.

## Full-triplet face staging

The `f` dispatch reaches `0x00109A30`. The full-triplet paths compare the
token-vector size to 10 (triangle) and 13 (quad), including the directive token.
The triangle loop at `0x00109E14..0x00109EF4` converts each corner's three
strings with decimal base 10 and subtracts one at `0x00109EBC..0x00109EC4`
before writing the three staging words. The quad's initial loop does the same
at `0x00109B1C..0x00109B38`.

The quad's first loop visits corners 0, 1, 2; the second loop at
`0x00109B5C..0x00109C54` visits corners 2, 3, 0. Thus its staged order is
(0,1,2),(2,3,0). Native convex-fan assembly currently uses (0,1,2),(0,2,3),
which describes the same oriented second triangle but does not preserve exact
index order. Do not claim byte-identical index streams.

These specific loops prove direct decrement at staging, not standard relative
negative-index semantics. Later lookup/normalization, partial-reference token
paths, missing normals, dedup equality and arbitrary polygons need further
proof. Their current native implementations remain explicitly port policies.

## First gameplay boundary remains open

The successfully loaded map records remain typed only at the file-layout and
extent level. Native rendering of one model does not prove entity placement or
mission simulation. Next recovery must follow map-record consumers and the
arena/update callers into typed native state before constructing a playable
scene. No gameplay facts are inferred from filename resemblance.
