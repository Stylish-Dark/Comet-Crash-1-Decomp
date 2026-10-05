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

## OBJ source references and vertex splitting

The temporary face-reference vector has exact 0x0C-byte records in internal
`position / normal / texcoord` order. The parser accepts only three shipped
face shapes: `v//vn` triangles, full `v/vt/vn` triangles, and full
`v/vt/vn` quads. Every present decimal source index is converted by exactly
`parsed_index - 1`; the original loader does not implement Wavefront negative
relative-index normalization.

Quads are triangulated as `0,1,2` and `2,3,0`. Missing texcoords use
`-1`.

The later collapse is not a global tuple-deduplication pass. One base vertex is
created per OBJ position. The first face corner claims that base vertex's
normal/UV slots. A later corner reuses the base only when the relevant
attributes match within the exact ~`0.0001f` tolerance; otherwise the loader
appends a fresh split vertex. Previously appended split vertices are not
searched, so repeated mismatching corners can create repeated duplicates.

The final pass writes the rewritten reference index as a u16 into model
`+0x1C`. Native typed behavior is in
`model_obj_semantics.hpp/.cpp`; exact parser anchors and a full shipped-corpus
audit are in `docs/decomp/obj-source-indexing.md`.

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
| `+0x40` | `+0x50` | bump/normal texture resource (`bump`) |
| `+0x44` | `+0x54` | environment/cube texture resource (`cube`) |
| `+0x48` | `+0x58` | standard `.vpo` vertex program; nonzero suppresses default selection |
| `+0x4C` | `+0x5C` | batched `_spu.vpo` vertex program |
| `+0x50` | `+0x60` | `.fpo` fragment program |
| `+0x54` | `+0x64` | legacy batched-path availability byte |

The MTL loader independently stores `map_Kd/map_Ks/bump/cube` resources into
material `+0x38/+0x3C/+0x40/+0x44`. Program helper `0x00101960` then expands
the selected shader base into standard `.vpo`, batched `_spu.vpo`, and
fragment `.fpo` resources at material `+0x48/+0x4C/+0x50`. The renderer
selects `+0x48` for the ordinary path or `+0x4C` for the batched path and
always pairs it with `+0x50`. See `docs/decomp/material-textures.md` and
`docs/decomp/material-programs.md`.

The exact shader decision tree remains unchanged from the previous recovery,
but it is now correctly exposed as **material** policy:

- `decomp/include/comet/material_shader_policy.hpp`
- `decomp/src/material_shader_policy.cpp`

## Indexed draw-range fields in each 0x78-byte submesh

Renderer `0x00100750` calls the indexed draw wrapper with
`GL_TRIANGLES (4)` and `GL_UNSIGNED_SHORT (0x1403)`. The argument mapping is
exactly compatible with `glDrawRangeElements(mode, start, end, count, type,
indices)`:

| submesh offset | native meaning | renderer use |
| ---: | --- | --- |
| `+0x00` | first index (u16 element offset) | multiplied by 2 and passed as the index-buffer byte offset |
| `+0x04` | index count | passed as draw count |
| `+0x08` | minimum referenced vertex | passed as draw-range start |
| `+0x0C` | maximum referenced vertex | passed as draw-range end |
| `+0x10` | material subobject | passed to the material binding helper |

This cleanly recovers the first 0x10 bytes of the submesh record as a normal
native `SubmeshDrawRange`.

## Batched/SPU path at +0x64..+0x74

Submesh `+0x64` is set by program helper `0x00101960` from whether the
batched `_spu.vpo` program at `+0x5C` loaded successfully. The remainder of
the tail is recovered from batch-preparation
function `0x000FEFB0`, renderer `0x00100750`, and the shipped
`*_spu.vpo` shaders:

| submesh offset | recovered meaning |
| ---: | --- |
| `+0x64` | byte-sized batch/SPU-path enable gate |
| `+0x68` | per-frame batched instance count |
| `+0x6C` | generated 32-byte-stride batched vertex stream |
| `+0x70` | generated u32 batched index stream |
| `+0x74` | generated 16-byte-per-instance `objInfo` stream |

The renderer's expanded draw uses
`instance_count * model_vertex_count` vertices and
`instance_count * submesh_index_count` indices. The batch coordinator resets
`+0x68` after the frame's prepared work is consumed.

See `docs/decomp/model-batching.md` for the exact preparation/draw anchors and
the original `normal_ty` / `objInfo` shader metadata.

## Additional promoted model fields

The remaining adjacent loader floats are no longer opaque:

| model offset | recovered meaning |
| ---: | --- |
| `+0x28` | geometry Y offset, applied after geometry scale when option mask `0x20` is set |
| `+0x2C` | signed bounding radius |

The loader's two float arguments are now `geometry_scale` and
`signed_radius_scale`; see `docs/decomp/model-load-parameters.md`.

A string-like member beginning at model `+0x34` still contains the source path
and remains represented as an ordinary native path/string rather than a fixed
PS3 STL layout.

## Material block completion

The former material gaps are now resolved as well:

- `+0x0C/+0x1C/+0x2C` are the zero-initialized W lanes of the ambient,
  diffuse, and specular vec4 shader inputs;
- `+0x34` is the integer/boolean `useTeamColor` field derived from a
  `newmtl` name beginning with `team`.

See `docs/decomp/material-properties.md` for the exact initialization,
renderer parameter names, and MTL-name stores.

## Native boundary and completion state

The meaningful 0x78-byte submesh fields are now structurally accounted for:
static draw range, full material properties/textures, standard and batched
vertex programs, fragment program, batched-path availability, and transient
batch streams. Exact OBJ source-index conversion and vertex-splitting behavior
are also recovered.

`decomp/include/comet/model_submesh.hpp` now provides the normal native
`ModelSubmesh + ModelMaterial` boundary. It intentionally carries semantic
properties, paths and shader-base/program specifications rather than fixed PS3
offsets, Cg/PSGL resource pointers or SPU output buffers.

The legacy SPU-expanded batch implementation remains an oracle only. A PC
renderer should map the recovered instance semantics to native GPU instancing
where possible.
