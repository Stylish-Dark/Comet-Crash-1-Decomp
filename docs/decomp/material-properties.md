# MTL scalar/color material properties

The same MTL parser that owns `map_Kd/map_Ks/bump/cube` also exposes four
standard Wavefront material directives with direct stores into the active
material object.

The material begins at `submesh + 0x10`; all offsets below are
material-relative.

| directive | keyword load | destination | meaning |
| --- | ---: | ---: | --- |
| `Ka` | `0x001091FC` | `+0x00/+0x04/+0x08` | ambient RGB |
| `Kd` | `0x0010932C` | `+0x10/+0x14/+0x18` | diffuse RGB |
| `Ks` | `0x0010A278` | `+0x20/+0x24/+0x28` | specular RGB |
| `Ns` | `0x0010A5D4` | `+0x30` | scaled specular exponent |

These semantic names are not guesses: `Ka`, `Kd`, `Ks`, and `Ns` are
the literal MTL keywords surviving in the executable, and each path immediately
parses the expected number of floating values.

## Exact write sites

`Ka` parses three floats and stores them at:

```text
0x00109288 -> material+0x00
0x001092C8 -> material+0x04
0x001092F0 -> material+0x08
```

`Kd` writes:

```text
0x0010939C -> material+0x10
0x001093DC -> material+0x14
0x00109404 -> material+0x18
```

`Ks` writes:

```text
0x0010A304 -> material+0x20
0x0010A344 -> material+0x24
0x0010A36C -> material+0x28
```

`Ns` parses one float, multiplies it by the exact binary constant
`0.12800000607967377`, and stores the single-precision result:

```text
0x0010A658  load scale constant
0x0010A65C  multiply Ns * scale
0x0010A670  store material+0x30
```

The native recovery therefore exposes `scale_mtl_specular_exponent()` with
the exact original multiplier rather than silently converting to a different
lighting convention.

## Material layout now recovered

Combining scalar/color and texture work gives the following proven region:

```text
+0x00 .. +0x08   ambient RGB (Ka)
+0x10 .. +0x18   diffuse RGB (Kd)
+0x20 .. +0x28   specular RGB (Ks)
+0x30            scaled specular exponent (Ns)
+0x38            diffuse texture resource (map_Kd)
+0x3C            specular texture resource (map_Ks)
+0x40            bump/normal texture resource (bump)
+0x44            environment cube resource (cube)
+0x48            shader resource / preassigned shader
```

The gaps at `+0x0C/+0x1C/+0x2C/+0x34` are intentionally left unnamed.
They may be alpha/padding/other material state, but current evidence does not
justify promoting them.

Native files:

- `decomp/include/comet/material_properties.hpp`
- `decomp/src/material_properties.cpp`
- `decomp/include/comet/material_textures.hpp`
- `decomp/include/comet/material_shader_policy.hpp`
