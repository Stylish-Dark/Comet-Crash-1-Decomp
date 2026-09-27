# Expanded submesh batching path

The alternate renderer path at **PPU `0x00100750`** is now sufficiently
constrained to model as a native transient batching system rather than an
unknown second mesh format.

This recovery uses three independent sources:

1. the exact title PPU renderer;
2. the exact title batch-builder at `0x000FEFB0`;
3. the original game's vertex-program binaries from the user's game data,
   whose parameter tables retain `position_tx`, `normal_ty`, and
   `objInfo`.

No proprietary shader bytes are committed.

## Legacy submesh tail

Each ordinary submesh record is 0x78 bytes.  The tail is:

| submesh offset | recovered role | exact evidence |
| ---: | --- | --- |
| `+0x64` | batching-enabled byte | `0x000FF060` loads it and skips all expanded-batch construction when zero |
| `+0x68` | transient batch instance count | reset at `0x000FF068`; written at `0x000FF12C`; renderer tests it at `0x001007E8` |
| `+0x6C` | packed expanded vertex stream | stored at `0x000FF134`; renderer binds it at `0x00100884` |
| `+0x70` | expanded u32 index stream | stored at `0x000FF138`; renderer consumes it at `0x001009C8` |
| `+0x74` | per-object `objInfo` vec4 stream | stored at `0x000FF13C`; bound to shader parameter `objInfo` at `0x00100938` |

The exact original field names are unknown, but the semantics above are
directly demonstrated by both writer and reader.

## Why `+0x68` is an instance/copy count

The high-level builder at `0x000FF3E0` iterates the same 40-slot model table
already recovered at root `+0x2D2DC0`, using the exact 0x90 model-object
stride.  For each active model slot it passes an active-object range into
`0x000FEFB0`.

Inside `0x000FEFB0`, the value ultimately stored at submesh `+0x68` controls
all three transient stream sizes.  The renderer later performs:

```text
draw_index_count = batch_count * submesh.index_count
draw_max_vertex  = batch_count * model.vertex_count - 1
index_type       = GL_UNSIGNED_INT
```

at `0x001009C0..0x001009E8`.

After the render cycle, `0x000FF594..0x000FF64C` walks the model/submesh table
again and clears every `submesh+0x68` back to zero.  This is transient
per-frame batching state, not permanent model geometry.

## Stream allocation equations

Writer `0x000FEFB0` budgets and advances three dynamic streams.

For `N = batch_count`, `V = model.vertex_count`, and
`I = submesh.index_count`:

```text
packed vertex bytes = N * V * 32
u32 index bytes      = N * I * 4
objInfo bytes        = N * 16
```

The combined vertex + object-info cursor advance is explicit at
`0x000FF370..0x000FF388`:

```text
N * (V * 32) + N * 16
```

The index cursor advance is explicit at `0x000FF130..0x000FF148`:

```text
N * (I * 4)
```

These equations are encoded by `ExpandedBatchCounts`.

## Packed expanded vertex format

The renderer binds the stream at `+0x6C` twice:

```text
stream + 0x00 : 4 floats, stride 32  -> fixed POSITION input
stream + 0x10 : 4 floats, stride 32  -> shader parameter "normal_ty"
```

The original vertex-program parameter table names the first input
`position_tx` and the second `normal_ty`.  Therefore the recovered native
layout preserves those exact names as two vec4s:

```text
0x00  vec4 position_tx
0x10  vec4 normal_ty
stride 0x20
```

The identifiers strongly encode the original packing convention, but the
native public type deliberately does not yet split either vec4's fourth
component into a separately named scalar until the producer side is fully
recovered.

## Per-object `objInfo`

The `+0x74` stream is bound as:

```text
4 floats, stride 16 -> shader parameter "objInfo"
```

The renderer then configures that parameter using model `vertex_count` before
the multiplied draw.  Combined with the allocation equation of exactly
`N * 16`, this proves there is one vec4 `objInfo` record per batched object,
not one per expanded vertex.

## SPU/SPURS-facing producer

`0x000FEFB0` does not directly expand every output vertex on the PPU.  It
builds one 256-byte work descriptor per chunk and calls the single helper at
`0x00138248`.  That helper rounds/caps DMA-sized regions and records the
source/destination ranges used by the batch work.

This is the strongest evidence yet that Comet Crash's previously unidentified
small active SPU workload is related to arena graphics/batching, but the exact
link to that embedded SPU image is **not yet proven**.  The native rewrite will
not depend on reproducing this PS3 job mechanism: once the producer transform
is completely recovered, it can be expressed directly as host/GPU batching.

## Native representation

- `decomp/include/comet/submesh_batch.hpp`
- `ExpandedBatchCounts`
- `ExpandedBatchVertexLayout`
- `ExpandedObjectInfoLayout`
- `ExpandedSubmeshBatch`

This converts `+0x64..+0x74` from opaque words into a renderer-facing native
description while preserving addresses/strides only as provenance.
