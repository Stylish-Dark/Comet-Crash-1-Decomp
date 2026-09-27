# MTL scalar/color material properties

The same MTL parser that owns `map_Kd/map_Ks/bump/cube` exposes four standard
Wavefront material directives plus one game-specific material-name policy.

The material begins at `submesh + 0x10`; all offsets below are
material-relative.

| directive/source | destination | meaning |
| --- | ---: | --- |
| `Ka` | `+0x00/+0x04/+0x08` | ambient RGB |
| record initialization | `+0x0C` | ambient vec4 W, initialized to 0 |
| `Kd` | `+0x10/+0x14/+0x18` | diffuse RGB |
| record initialization | `+0x1C` | diffuse vec4 W, initialized to 0 |
| `Ks` | `+0x20/+0x24/+0x28` | specular RGB |
| record initialization | `+0x2C` | specular vec4 W, initialized to 0 |
| `Ns` | `+0x30` | scaled specular exponent |
| `newmtl team*` policy | `+0x34` | integer/boolean `useTeamColor` |

## Ka/Kd/Ks/Ns write sites

`Ka` parses three floats:

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

## The former +0x0C/+0x1C/+0x2C gaps are vec4 W lanes

When a new 0x78-byte submesh/material record is created, the loader zeroes the
entire material region before parsing at `0x00107150..0x001071B4`.

The Ka/Kd/Ks parsers then write exactly three floats apiece, leaving
`+0x0C/+0x1C/+0x2C` untouched at zero.

Renderer material binder `0x0010000C` independently proves why those lanes
exist. It looks up the surviving Cg parameter names:

```text
colorDiffuse
colorAmbient
colorSpecular
```

and uploads pointers to material `+0x10`, `+0x00`, and `+0x20` through
the vec4 parameter path. Therefore the layout is three 16-byte color vectors;
the fourth lane is a real shader input slot but is zero on the recovered MTL
load path. It is named **W**, not alpha, because no evidence assigns alpha
semantics.

## +0x34 = useTeamColor

The remaining material gap is directly consumed by renderer `0x0010000C`.

At `0x00100084` the executable looks up shader parameter
`"useTeamColor"`. It loads material `+0x34` at `0x0010008C`, converts
that integer to float, and uploads it when the parameter exists.

The MTL `newmtl` path compares the material name with literal `"team"`
using at most four characters:

```text
0x00109758  load literal "team"
0x00109760  compare material-name prefix
...
0x001097B8  store 1 -> material+0x34 on match
0x0010A944  store 0 -> material+0x34 otherwise
```

The original shipped MTL files contain both `newmtl team` and
`newmtl team.light`, matching the recovered prefix behavior.

Native code exposes this as `MaterialProperties::use_team_color` and
`material_name_uses_team_color()`.

## Material layout now recovered

Combining scalar/color, texture and shader work gives the material block:

```text
+0x00 .. +0x0C   ambient vec4 (Ka RGB + zero W)
+0x10 .. +0x1C   diffuse vec4 (Kd RGB + zero W)
+0x20 .. +0x2C   specular vec4 (Ks RGB + zero W)
+0x30            scaled specular exponent (Ns)
+0x34            useTeamColor integer/boolean
+0x38            diffuse texture resource (map_Kd)
+0x3C            specular texture resource (map_Ks)
+0x40            bump/normal texture resource (bump)
+0x44            environment cube resource (cube)
+0x48            shader resource / preassigned shader
```

This removes the four remaining anonymous gaps from the material region.

Native files:

- `decomp/include/comet/material_properties.hpp`
- `decomp/src/material_properties.cpp`
- `decomp/include/comet/material_textures.hpp`
- `decomp/include/comet/material_shader_policy.hpp`
