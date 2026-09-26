# Model vertex Y-offset option

One remaining root-model field near the geometry metadata is now recovered:
**model `+0x28` is an optional Y offset applied while parsing OBJ vertices.**

## Exact parser evidence

The relevant block is entered only for a single-character OBJ directive whose
first byte is literal `0x76` (`'v'`):

```text
0x001074EC  load first character
0x001074F0  subtract 0x76 ('v')
...
0x00107518  enter position parse
```

The loader parses three floats, multiplies all three by the already-recovered
geometry scale, and stores x/y/z into the temporary position:

```text
0x001075A4  store scaled component 0 (x)
0x001075EC  store scaled component 1 (y)
0x0010762C  store scaled component 2 (z)
```

Immediately after the z store, it tests one exact loader option bit:

```text
0x0010761C  load original model option word
0x00107624  isolate option bit 0x20
0x00107630  branch when set
```

The taken branch lands at `0x0010AB3C`. There the address arithmetic points
to the **second** component of the just-parsed position (temporary vertex
start + 4), then performs:

```text
0x0010AB40  load model+0x28
0x0010AB4C  load current Y
0x0010AB50  Y += model+0x28
0x0010AB54  store adjusted Y
```

Thus:

```cpp
if (model_options & 0x20)
    position.y += model.vertex_y_offset;
```

## Native representation

The field is now promoted as:

- `LegacyModelGeometryOffsets::vertex_y_offset = 0x28`
- `kLegacyModelOptionApplyVertexYOffset = 0x20`
- `apply_optional_vertex_y_offset()`

The bit is deliberately kept as a legacy numeric option rather than inventing
an original enum constant name.

The loader applies this offset **after geometry scaling** and before the
signed-bounding-radius calculation. This ordering matters for a faithful
native parser.
