# Arena asset/bootstrap recovery

This document covers the model/font/shader bootstrap cluster in PPU function
**`0x000ECCA8..0x000EE87C`** (next OPD function begins at `0x000EE880`).

The function is large, but the model-loading half now has a clear native shape:
it is a declarative resource manifest encoded procedurally in the original
PowerPC code.

## High-level bootstrap sequence

Surviving strings show the routine initializes, in order:

- fonts: `wmd_font_b.fnt`, `wmd_font_icons.fnt`,
  `wmd_font_readable.fnt`, `wmd_font_readable_xs.fnt`;
- font shaders: `font_outlined_shader`, `font_rgba_shader`;
- `shaders.bin`;
- particle textures:
  `images/particle_effects_size.dds` and
  `images/particle_effects.dds`;
- a large contiguous table of arena/gameplay model objects;
- `gui_quad_shader` near the end of the function.

The font/shader helper signatures still need semantic recovery. The model table
is much stronger and is now represented natively.

## Model-loader operation at PPU `0x00105308`

There are exactly **37 direct calls** to `0x00105308` in this bootstrap.

Each call has the same argument shape:

```text
r3 = destination model object
r4 = integer option/flag word
r5 = path string
f1 = floating parameter 1
f2 = floating parameter 2
```

The callee stores the integer option into the destination object immediately and
then enters a large model/path initialization/parser path. It is therefore safe
to treat `0x00105308` as the model-asset initializer boundary for the native
rewrite, while retaining its exact original source name as unknown.

The two floating parameters are preserved verbatim in the recovered manifest.
They are **not** called scale/size/etc. yet because their exact source-level
meaning is not proven.

## Model object table

Before model initialization, `0x000ED13C` forms:

```text
r25 = root_game_state + 0x2D0000
```

Every model destination is then `r25 + low_offset`. All 37 destinations land
on a strict **0x90-byte stride** beginning at root offset **`0x2D2DC0`**.

That makes the original layout:

```text
slot N address = root + 0x2D2DC0 + N * 0x90
```

The bootstrap touches slots 0..33, 35, 36 and 39. Slots 34, 37 and 38 are not
initialized by these 37 model calls and remain targets for later cross-reference
recovery.

This is strong evidence for a contiguous original model/resource object array
whose element size is 0x90. The native port does not preserve that ABI; slot
indices and offsets are provenance metadata.

## Recovered manifest

The exact 37-entry model list is checked in as:

- `decomp/include/comet/arena_assets.hpp`
- `decomp/src/arena_assets.cpp`

It records:

- original asset path;
- original root-state offset;
- original 0x90-table slot;
- exact integer option word passed to `0x00105308`;
- the exact two floating arguments.

Representative entries:

| slot | root offset | path | options | p1 | p2 |
| ---: | ---: | --- | ---: | ---: | ---: |
| 0 | `0x2D2DC0` | `playerShip.obj` | `0x003` | 1.30 | 0.85 |
| 2 | `0x2D2EE0` | `scout02.obj` | `0x063` | 1.12 | 0.88 |
| 11 | `0x2D33F0` | `structTurretStand.obj` | `0x043` | 1.00 | 0.00 |
| 23 | `0x2D3AB0` | `structBasePlat.obj` | `0x043` | 1.00 | -1.00 |
| 28 | `0x2D3D80` | `platBasic.obj` | `0x002` | 1.00 | -0.90 |
| 36 | `0x2D4200` | `resourceGeode.obj` | `0x245` | 3.3333 | 1.00 |
| 39 | `0x2D43B0` | `sun.obj` | `0x000` | 1.00 | 0.00 |

The full list should be used as the native bootstrap input rather than
reproducing 37 hand-written PS3-era call sequences.

## Important exact observations

- Four consecutive slots 28..31 all load the same
  `models/care/platforms/platBasic.obj` asset.
- Slot 32 loads `platAdv.obj`.
- Weapon/structure assets occupy a dense region of the same 0x90 table rather
  than a separate allocator/type family.
- `resourceGeode` and `sun` are far later slots in the same regular table,
  independently strengthening the array interpretation.
- `playerSquare2.obj` is slot 33 and `gateway.obj` is slot 35; the missing
  slot 34 between them is real, not a stride mistake.

## Next recovery boundary

The next useful semantic cut is the implementation at `0x00105308` itself.
Recovering its 0x90-byte object layout will tell us what the option bits and
floating arguments actually mean and will convert the manifest from
"faithfully preserved call data" into a typed native model resource API.

The font/shader setup in the first half of `0x000ECCA8` remains a parallel
target after that boundary is understood.
