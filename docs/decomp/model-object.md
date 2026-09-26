# Model geometry and submesh/material recovery

This document records the current semantic recovery of the 0x90-byte arena
model object loaded by PPU `0x00105308..0x0010B537` and rendered by
`0x00100750`.

A correction to the previous pass is important: helper
`0x00105088..0x00105307` does **not** receive the root model object. It
receives the material subobject at `submesh + 0x10`. The earlier
`+0x38/+0x3C/+0x40/+0x48` shader fields are therefore material-subobject
offsets, not root-model offsets.

## Proven top-level model geometry fields

The OBJ loader and renderer independently agree on the following layout:

| model offset | recovered meaning | evidence |
| ---: | --- | --- |
| `+0x00` | original model option word | written from loader argument near entry; later reused for material shader policy |
| `+0x04` | unique vertex count | derived from a temporary 0x20-byte vertex vector and used by renderer-side vertex setup |
| `+0x08` | submesh/material-record count | derived from a temporary vector with exact 0x78-byte stride; renderer loops exactly this many records |
| `+0x0C` | final index count | derived from 0x0C-byte OBJ face-reference triplets; final buffer allocation is count*2 |
| `+0x10` | full vertex-array pointer | loader allocates `vertex_count * 0x20`; renderer chooses the 32-byte format when nonzero |
| `+0x14` | compact vertex-array pointer | alternate loader path allocates `vertex_count * 0x14`; renderer uses it when `+0x10 == 0` |
| `+0x18` | submesh-record pointer | points to `submesh_count` records of 0x78 bytes |
| `+0x1C` | 16-bit index-array pointer | loader allocates `index_count * 2`; renderer issues `GL_UNSIGNED_SHORT (0x1403)` indexed draws |
| `+0x20` | GPU array-buffer handle for full layout | renderer binds it to `GL_ARRAY_BUFFER (0x8892)` on the 32-byte path |
| `+0x24` | GPU array-buffer handle for compact layout | renderer binds it to `GL_ARRAY_BUFFER` on the 20-byte path |

The native rewrite should turn these into ordinary vectors/resources rather than
preserving pointer/PSGL-handle fields at fixed offsets.

## Recovered vertex layouts

The full path is exactly 32 bytes per vertex:

```text
+0x00  float position[3]
+0x0C  float normal[3]
+0x18  float texcoord[2]
stride 0x20
```

The compact path is exactly 20 bytes per vertex:

```text
+0x00  float position[3]
+0x0C  float texcoord[2]
stride 0x14
```

The renderer at `0x00100750` selects between them by testing model
`+0x10`. When present it binds model `+0x20` and uses the 0x20 layout.
Otherwise it binds model `+0x24` and uses the 0x14 layout.

This strongly ties the alternate path to geometry without a normal stream.
That is now expressed directly by `full_vertex_layout()` and
`compact_vertex_layout()`.

## OBJ face-reference collapse

During parsing, the loader maintains a temporary vector with an exact
**0x0C-byte stride**. Its count becomes model `+0x0C`.

The subsequent deduplication pass treats each 12-byte element as the source
reference used to find/build one unique interleaved vertex and writes one
16-bit final index into model `+0x1C`.

That structure matches the OBJ `v/vt/vn` face-reference triplet shape. The
native parser should eventually represent it as a typed source-index triplet,
but the exact signed/one-based normalization rules remain to be recovered
before freezing the public type.

## Submesh records

Model `+0x18` points to records with exact **0x78-byte stride**.

The renderer at `0x00100750` advances by 0x78 for each iteration and stops
after model `+0x08` records. The loader computes that count from the temporary
0x78-byte vector and copies the records into the final allocation.

A material/shader subobject begins at **submesh + 0x10**. This is independently
proved twice:

1. the OBJ/MTL loader calls shader helper `0x00105088` with
   `material_vector_end - 0x68`; since the record stride is 0x78, that address
   is the last record start + 0x10;
2. the renderer calls material binding helper `0x0010000C` with
   `submesh + 0x10`.

## Corrected material shader fields

Within the material subobject, helper `0x00105088` uses:

| material-relative | submesh-relative | recovered meaning |
| ---: | ---: | --- |
| `+0x38` | `+0x48` | diffuse/base texture present |
| `+0x3C` | `+0x4C` | specular texture present |
| `+0x40` | `+0x50` | bump/normal texture present |
| `+0x48` | `+0x58` | preassigned shader; nonzero suppresses default selection |

The renderer also reads submesh `+0x4C/+0x50/+0x58`, matching these roles.

The exact shader decision tree remains unchanged from the previous recovery,
but it is now correctly exposed as **material** policy:

- `decomp/include/comet/material_shader_policy.hpp`
- `decomp/src/material_shader_policy.cpp`

## Fields not promoted yet

The loader also touches model `+0x28`, `+0x2C` and a string-like member
beginning at `+0x34`. Their exact native meanings are not yet strong enough to
name.

The second floating loader argument influences `+0x2C`, including a sign
inversion path. The first floating argument is retained by the loader but its
final semantic effect still needs to be traced.

## Next boundary

Continue through the loader after geometry collapse to recover:

- how OBJ position/texcoord/normal source arrays are normalized;
- exact submesh draw-range fields at `+0x00..+0x0C` and `+0x68..+0x74`;
- the meaning of `+0x28/+0x2C`;
- material texture construction from `map_Kd`, `map_Ks`, `bump` and
  `cube` MTL directives.

The goal is to replace the remaining 0x78-byte opaque submesh record with a
native typed `Submesh + Material` representation.
