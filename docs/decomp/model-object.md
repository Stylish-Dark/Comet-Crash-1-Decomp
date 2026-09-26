# Model object / default shader recovery

This file records the first typed slice of the 0x90-byte model object used by
the arena asset manifest.

## Boundary

Arena bootstrap function `0x000ECCA8` calls the large model initializer
`0x00105308..0x0010B537` 37 times.

The initializer stores the caller's integer option word at model offset
`+0x00`, copies/constructs the path into the model object, parses the model,
then later calls helper **`0x00105088..0x00105307`** with:

```text
r3 = model object
r4 = original option word
```

Unlike the full loader, `0x00105088` is compact enough to recover completely.

## Proven model fields

The helper tests four 32-bit object fields:

| model offset | recovered meaning | evidence |
| ---: | --- | --- |
| `+0x38` | diffuse/base texture present | with only this field present it selects `lit_texture_shader` |
| `+0x3C` | specular texture present | adding this field selects `lit_texture_spec_shader` |
| `+0x40` | bump/normal texture present | adding this without spec selects `lit_bump_shader`; with spec it selects bump+spec variants |
| `+0x48` | shader already assigned | nonzero causes an immediate return without default selection |

These names are supported by the exact surviving shader strings and the
branch matrix, rather than guessed from address proximity.

## Exact default-shader decision tree

The native rewrite is in:

- `decomp/include/comet/model_shader_policy.hpp`
- `decomp/src/model_shader_policy.cpp`

The original shader strings are:

```text
lit_object_shader
lit_object_shader_no_team_ground
lit_texture_shader
lit_bump_shader
lit_texture_spec_shader
lit_texture_spec_gloss_shader
lit_bump_spec_shader
lit_bump_spec_shader_no_team
lit_bump_spec_shader_no_team_ground
lit_bump_spec_gloss_shader_no_team
lit_bump_spec_gloss_glow_shader_no_team
```

The exact policy is:

```text
if shader already assigned:
    unchanged

if no diffuse:
    option bit 0x4 ? object_no_team_ground : object

else if no specular:
    bump present ? bump : texture

else if no bump:
    option bit 0x40 ? texture_spec_gloss : texture_spec

else:
    if (options & 0x0C) == 0x0C:
        bump_spec_no_team_ground
    else if (options & 0x244) == 0x244:
        bump_spec_gloss_glow_no_team
    else if (options & 0x44) == 0x44:
        bump_spec_gloss_no_team
    else if options & 0x4:
        bump_spec_no_team
    else if options & 0x40:
        unchanged
    else:
        bump_spec
```

The option-bit semantics themselves are deliberately not renamed yet. The
shader names strongly suggest team/gloss/glow behaviour, but the project will
wait for independent use sites before turning bit values into source-level
enum names.

## Initial `0x00105308` object facts

The large initializer already yields several additional hard facts:

- `+0x00` receives the exact bootstrap option word;
- a string-like member begins at `+0x34` and receives the source path near
  function entry;
- `+0x2C` is a float field influenced by the second floating argument;
- the initializer later populates model data/count/pointer fields around
  `+0x04..+0x1C`;
- `+0x38/+0x3C/+0x40/+0x48` form the material/default-shader portion above.

The next pass should identify the pointer/count pairs at `+0x04..+0x1C` and
follow the OBJ-text parser sufficiently to name geometry arrays and material
records.
