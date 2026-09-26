# MTL texture directive recovery

The MTL portion of model loader `0x00105308..0x0010B537` now has four exact
texture directives tied to four adjacent fields in the material subobject.

The material subobject begins at `submesh + 0x10`. Offsets below are relative
to that material object.

| directive | code anchor | material field | loader path | recovered role |
| --- | ---: | ---: | --- | --- |
| `map_Kd` | `0x0010A6A0` | `+0x38` | `0x0010EE20` | diffuse/base 2D texture |
| `map_Ks` | `0x0010AC80` | `+0x3C` | `0x0010EE20` | specular 2D texture |
| `bump` | `0x0010AF18` | `+0x40` | `0x0010EE20` | bump/normal 2D texture |
| `cube` | `0x0010B1AC` | `+0x44` | `0x0010F100` | environment/cube texture |

## Exact store evidence

After each successful directive parse the original loader constructs a texture
resource and writes its returned handle/object pointer directly into the active
material:

```text
map_Kd -> stw returned_texture, material+0x38  @ 0x0010A8F8
map_Ks -> stw returned_texture, material+0x3C  @ 0x0010AED8
bump   -> stw returned_texture, material+0x40  @ 0x0010B174
cube   -> stw returned_texture, material+0x44  @ 0x0010B368
```

The first three directives share the same resource-loader path. `cube` uses a
distinct loader, matching its different texture topology.

This upgrades the earlier shader-policy evidence: fields `+0x38/+0x3C/+0x40`
are not merely boolean "texture present" flags. They are the actual legacy
material texture resource fields; shader selection tests them for null/non-null.

## Native representation

`decomp/include/comet/material_textures.hpp` exposes the semantic directive
mapping without preserving the original PowerPC parser or PSGL texture object
layout.

The next native OBJ/MTL parser should map these four directives directly into a
typed Material structure:

```text
Material
  diffuse_texture
  specular_texture
  bump_texture
  environment_cube
  shader
```

The exact filename/path-token cleanup around the directives is still being
recovered; the field ownership and texture-channel mapping above are proven.
