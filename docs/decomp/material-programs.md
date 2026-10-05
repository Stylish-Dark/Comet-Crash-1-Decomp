# Material GPU-program recovery

Material helper `0x00101960` receives the material subobject at `submesh+0x10`
and a shader base name selected by `0x00105088`. It expands that base into
three concrete program filenames and loads all three resources.

## Exact legacy fields

| material offset | submesh offset | recovered meaning | filename |
| ---: | ---: | --- | --- |
| `+0x48` | `+0x58` | standard vertex program | `<base>.vpo` |
| `+0x4C` | `+0x5C` | batched/SPU vertex program | `<base>_spu.vpo` |
| `+0x50` | `+0x60` | fragment program | `<base>.fpo` |
| `+0x54` | `+0x64` | batched-path enable byte | nonzero when the `_spu.vpo` resource loaded |

The surviving suffix literals are `.vpo`, `_spu.vpo`, and `.fpo`. Native
`material_program_paths()` reproduces only this filename policy; it does not
carry the original PSGL/Cg resource objects into the PC architecture.

## Store anchors in `0x00101960`

After constructing the standard vertex filename, the helper loads it through
the vertex-program resource path and stores the result:

```text
0x00101DB0  load standard .vpo resource
0x00101DBC  store -> material+0x48
```

The batched filename is constructed by appending the surviving `_spu.vpo`
literal to the same shader base. It uses the same vertex-program loader:

```text
0x00101EDC  load batched _spu.vpo resource
0x00101EE4  store -> material+0x4C
0x00101EFC  read material+0x4C
0x00101F20  store boolean resource-present result -> material+0x54
```

The fragment filename uses `.fpo` and the fragment-program resource path:

```text
0x00101F24  load fragment .fpo resource
0x00101F30  store -> material+0x50
```

## Renderer selection proves the roles

Material binder `0x0010000C` receives a path selector in its second argument.
For the ordinary model path, it selects material `+0x48`; for the batched path
it selects material `+0x4C`. Both paths always pair the selected vertex program
with material `+0x50`:

```text
0x00100054  ordinary-path branch
0x0010055C  load material+0x48 standard vertex program
0x00100058  load material+0x4C batched vertex program
0x00100060  load material+0x50 fragment program
0x00100070  bind selected vertex program
0x0010007C  bind fragment program
```

Renderer `0x00100750` passes the ordinary selector for the static u16-index
path and the batched selector when `submesh+0x68` contains prepared instances.
It also directly uses `submesh+0x5C` for the batched shader-parameter lookup,
independently confirming that the middle program field is the `_spu.vpo`
variant.

## Architectural consequence

The final PC renderer does not need three PSGL program handles embedded in a
material. The recovered native boundary is a shader-base/program specification
from which a PC backend can create its own pipeline objects. The `+0x64` byte
is likewise provenance for availability of the legacy batched vertex variant,
not a requirement to reproduce SPU-expanded geometry.
