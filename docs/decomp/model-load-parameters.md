# Model-loader floating parameters

The two floating arguments supplied by all 37 arena model bootstrap calls are
now recoverable from the data flow of PPU `0x00105308`.

They are no longer opaque `param1/param2` values.

## f1 — geometry scale

At entry:

```text
0x001053A4  fmr f29,f1
```

A control-flow/reaching-definition pass shows that the value reaching the OBJ
vertex parse block at `0x0010751C` is still exactly that entry definition.
The parser then multiplies every component of an OBJ `v x y z` position by
the value before storing it:

```text
0x0010751C  fmr   f31,f29
0x00107580  fmul  parsed_x, f31, parsed_x
0x001075A4  store scaled x
0x001075C4  fmul  parsed_y, f31, parsed_y
0x001075EC  store scaled y
0x00107610  fmul  parsed_z, f31, parsed_z
0x0010762C  store scaled z
```

The first floating argument is therefore **geometry scale**.

This resolves values in the arena manifest such as player ship `1.30`,
scout `1.12`, and resource geode `3.3333` as literal model geometry scales.

## f2 — signed bounding-radius scale

At entry the second float is retained and its magnitude is calculated:

```text
0x001053B4  fmr  f28,f2
0x00105448  fabs f30,f28
0x0010548C  model+0x2C = 0
```

For each scaled vertex, the loader computes the Euclidean distance from the
origin. The VMX sequence at `0x00107684..0x001076C4` is a vectorized
square-root calculation over `x*x + y*y + z*z`. It then performs:

```text
candidate = abs(f2) * length(position)
model+0x2C = max(model+0x2C, candidate)
```

with the multiply at `0x001076C8` and the update paths ending at
`0x001076D4` / `0x00108464`.

During finalization, the original sign of f2 is restored:

```text
0x00105D2C  compare f2 with 0
0x00105D4C  load model+0x2C
0x00105D50  negate when f2 < 0
0x00105D54  store model+0x2C
```

Thus model `+0x2C` is a **signed bounding radius** centered at the model
origin, and f2 is its signed scale.

The negative values used by platform/base assets are not discarded as odd
input; their sign is intentionally preserved by the original loader. The
gameplay meaning of a negative radius is a later cross-reference question, but
the loader mathematics itself is now exact.

## model +0x28 — geometry Y offset

The remaining adjacent root-model float is now resolved from the same OBJ
vertex-import block.

After x/y/z have been multiplied by `geometry_scale`, the loader reads the
model option word and isolates mask **`0x20`**:

```text
0x0010761C  load model+0x00 options
0x00107624  isolate option mask 0x20
0x00107628  compare masked value with zero in CR1
0x00107630  bf 6 -> 0x0010AB3C when CR1.EQ is false
```

CR bit 6 is CR1.EQ, so the branch is taken when the masked value is nonzero:
**option bit 0x20 is set**.

The target then reads `model+0x28` and adds it to the already-scaled Y
component:

```text
0x0010AB40  load model+0x28
0x0010AB44  address current vertex Y component
0x0010AB4C  load scaled Y
0x0010AB50  scaled Y + model+0x28
0x0010AB54  store adjusted Y
```

Therefore:

- model `+0x28` = **geometry Y offset**;
- model option mask `0x20` = **apply geometry Y offset during OBJ import**.

The original enum/flag identifier remains unknown, so the native code exposes
the descriptive constant `kModelOptionApplyGeometryYOffset` rather than
inventing an original source name.

## Native representation


- `ArenaModelAssetSpec::geometry_scale`
- `ArenaModelAssetSpec::signed_radius_scale`
- `ModelLoadParameters`
- `apply_geometry_scale()`
- `compute_signed_bounding_radius()`
- `ModelLoadParameters::geometry_y_offset`
- `kModelOptionApplyGeometryYOffset`
- `transform_imported_position()`

The native port can therefore stop carrying these as anonymous ABI arguments.
