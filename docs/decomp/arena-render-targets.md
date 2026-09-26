# `arenaGraphics.cpp` render-target recovery

This document covers PPU function **`0x000E5A34..0x000E65D4`** in NPEB00142 v1.00.

The function is no longer treated as an opaque graphics routine. Exact call/constant tracing identifies it as arena render-target/framebuffer setup. The original uses PSGL/OpenGL-shaped wrappers; the native port should preserve the target semantics and discard the PSGL state machine.

## Source-line anchors

Seven surviving source strings segment the routine:

| PPU instruction | Source marker | Immediately preceding completed setup |
| --- | --- | --- |
| `0x000E5C5C` | `arenaGraphics.cpp:1034` | scaled RGBA color target attached to FBO `+0x445C` |
| `0x000E5D60` | `arenaGraphics.cpp:1081` | depth texture attached to the same FBO |
| `0x000E5E3C` | `arenaGraphics.cpp:1100` | RGB16F target / FBO `+0x4468` |
| `0x000E5F24` | `arenaGraphics.cpp:1115` | 384x384 RGBA8 target / FBO `+0x4488` |
| `0x000E5FFC` | `arenaGraphics.cpp:1130` | RGB16F target / FBO `+0x446C` |
| `0x000E60D0` | `arenaGraphics.cpp:1143` | RGB16F target / FBO `+0x4470` |
| `0x000E6524` | `arenaGraphics.cpp:1188` | 80x64 multi-target FBO loop completion check |

Every marker is passed to helper `0x000E59AC`, whose only substantive operation is a framebuffer-completeness check. That helper calls `0x0002DC4C` with `GL_FRAMEBUFFER (0x8D40)`; the successful status is `GL_FRAMEBUFFER_COMPLETE (0x8CD5)`.

## Graphics-wrapper identities proven by use

| PPU function | Recovered operation | Evidence |
| --- | --- | --- |
| `0x0001C6E4` | bind 2D texture | call sites pass `GL_TEXTURE_2D (0x0DE1)` + a handle generated into texture slots |
| `0x0001E18C` | float texture parameter | pnames include `0x2800..0x2803`; float values include 9728, 9729, 10496 |
| `0x0001DB64` | integer texture parameter | same texture-parameter pnames and GL enum values |
| `0x0001C810` | 2D texture image/allocation | arguments line up as target, level, internal format, width, height, border, format, type, data |
| `0x0002DD58` | bind framebuffer | target `0x8D40`; bound handle is subsequently used by attachment/check code |
| `0x0002EA48` | framebuffer texture attachment | target `0x8D40`, attachments `0x8CE0/0x8CE1/0x8D00`, texture target `0x0DE1` |
| `0x0002DC4C` | framebuffer status | default/success result `0x8CD5` and bound-FBO status lookup |
| `0x0001C718` | generate/replace texture handles | output slots are later consumed exclusively as texture handles |
| `0x0002E634` | generate/replace framebuffer handles | output slots are later consumed as framebuffer handles |

`0x0002E068`, `0x0002B0F8`, `0x0002DD1C`, and `0x00011488` are deliberately not assigned final API names yet.

## Proven renderer-state handle slots

These offsets are relative to the root game state. They are concentrated in the `+0x2D44xx` renderer region.

### Texture handles

```text
+0x2D448C
+0x2D4490
+0x2D4494   alias/copy of +0x2D4490 in this setup path
+0x2D4498
+0x2D449C
+0x2D44A0
+0x2D44A4
+0x2D44A8
+0x2D44AC
+0x2D44B0
+0x2D44B4
+0x2D44B8
```

### Framebuffer handles

```text
+0x2D445C
+0x2D4460
+0x2D4468
+0x2D446C
+0x2D4470
+0x2D4474
+0x2D4478
+0x2D447C
+0x2D4480
+0x2D4484
+0x2D4488
```

The resource-generation routine immediately preceding `0x000E5A34` creates these handles, which independently confirms their class from later usage.

## Render-target blocks

The native target-plan representation is checked in as `decomp/include/comet/arena_render_targets.hpp` and `decomp/src/arena_render_targets.cpp`.

The following facts are exact. Semantic purpose names such as shadow/bloom/exposure are **not** assigned yet.

| Block | Texture offset | FBO offset | Dimensions | Internal | Format/type | Attachment |
| --- | ---: | ---: | --- | --- | --- | --- |
| base texture | `4490` | — | display W x H | `0x6007` | RGBA / U8 | — |
| scaled color | `4498` | `445C` | display W*scaleX x H*scaleY | `0x6007` | RGBA / U8 | COLOR0 |
| scaled depth | `448C` | `445C` | display W*scaleX x H*scaleY | DEPTH_COMPONENT | DEPTH / U32 | DEPTH |
| float target A | `449C` | `4468` | auxiliary W x H | RGB16F | RGB / HALF_FLOAT | COLOR0 |
| square target | `44B8` | `4488` | 384 x 384 | `0x6007` | RGBA / U8 | COLOR0 |
| float target B | `44A0` | `446C` | auxiliary W x H | RGB16F | RGB / HALF_FLOAT | COLOR0 |
| float target C | `44A4` | `4470` | auxiliary W x H | RGB16F | RGB / HALF_FLOAT | COLOR0 |
| small target A | `44AC` | `4480` | 80 x 64 | `0x6007` | RGBA / U8 | COLOR0 |
| small target B | `44B0` | `4484` | 80 x 64 | `0x6007` | RGBA / U8 | COLOR0 |
| MRT 0 | `44A8` + shared `44B4` | `4474` | 80 x 64 | `0x6007` final allocation | RGBA / U8 | COLOR0 + COLOR1 |
| MRT 1 | `44AC` + shared `44B4` | `4478` | 80 x 64 | `0x6007` | RGBA / U8 | COLOR0 + COLOR1 |
| MRT 2 | `44B0` + shared `44B4` | `447C` | 80 x 64 | `0x6007` | RGBA / U8 | COLOR0 + COLOR1 |

Before the common MRT loop, texture `+44A8` is also allocated once as 80x64 RGBA16F and attached to FBO `+4474`; the same texture is then redefined as RGBA8 by the common loop before the line-1188 completeness check. This behavior is preserved as an explicit **preflight allocation** in the recovered plan rather than silently “cleaned up” while its purpose is still unknown.

## Resolution/mode policy

The function reads two mutable display-size globals whose reference values are 720x480. The scaled color/depth pair multiply those dimensions according to a renderer mode:

| mode | scale X | scale Y | legacy texture parameter `0x6022` |
| ---: | ---: | ---: | ---: |
| 0 | 2 | 2 | `0x6033` |
| 1 | 2 | 2 | `0x6032` |
| 2 | 2 | 1 | `0x6031` |
| other | 1 | 1 | `0x6030` |

The meaning of PSGL-specific parameter `0x6022` is not yet promoted into the native API. The scale policy is.

A second pair of mutable globals has reference values 480x256 and supplies the dimensions of the three RGB16F targets. They are represented as `auxiliary_width/height` pending recovery of the original source names.

## Native boundary

The recovered target plan intentionally contains:

- dimensions;
- color/depth formats;
- filtering/wrap policy;
- texture/FBO provenance offsets;
- attachment relationships.

It intentionally does **not** expose PSGL function calls, RSX state or PS3 object handles as the native renderer API. Those addresses remain provenance metadata only.
