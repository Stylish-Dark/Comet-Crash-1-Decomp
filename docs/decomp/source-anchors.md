# Decompilation source anchors

This file records evidence that ties stripped PPU addresses to original game/source concepts. It is intentionally conservative: addresses and strings are facts; names beyond what the binary proves remain provisional.

## `arenaGraphics.cpp`

The exact NPEB00142 v1.00 executable retains seven source-line strings. All seven are referenced from one PPU function, `0x000E5A34`:

| PPU instruction | Embedded source marker |
| --- | --- |
| `0x000E5C5C` | `arenaGraphics.cpp:1034` |
| `0x000E5D60` | `arenaGraphics.cpp:1081` |
| `0x000E5E3C` | `arenaGraphics.cpp:1100` |
| `0x000E5F24` | `arenaGraphics.cpp:1115` |
| `0x000E5FFC` | `arenaGraphics.cpp:1130` |
| `0x000E60D0` | `arenaGraphics.cpp:1143` |
| `0x000E6288` | `arenaGraphics.cpp:1188` |

This establishes `0x000E5A34..0x000E65D4` as a large function originating in `arenaGraphics.cpp`, covering at least source lines 1034-1188. The next decompilation pass should reconstruct this function around those seven assertion/debug anchors and identify its direct callees before assigning a semantic function name.

## Level-map path builder

Function `0x000D5D5C..0x000D5E18` references the literal `"%slevel%u.map"` at instruction `0x000D5D98`.

Recovered behaviour:

```text
level_index = *(u32*)(game_state + 0x2D451C)
path = format("%slevel%u.map", global_level_path_prefix, level_index)
result = sub_000D91B0(game_state + 0x2D6438, level_index, path, (u8)mode)
return result != 0
```

A semantic C++ translation is checked in as `decomp/recovered/level_map_000D5D5C.cpp`. The root structure, path-prefix global and `0x000D91B0` callee remain intentionally unnamed until their surrounding usage is recovered.

## Asset/bootstrap cluster

Function `0x000ECCA8` references a large contiguous set of game assets, including fonts, `playerShip.obj`, resource geodes, agents, platforms, weapons, structures, mines, bullets and gateways. It is therefore a strong candidate for an arena/gameplay asset bootstrap routine. Do not assign a final function name until constructor/global-store behaviour is recovered.

## Tooling

`tools/decomp_source_refs.py` recovers TOC-mediated printable-string references and assigns them to PPU functions using `EBOOT.functions.json`. On the reference title it identifies the `arenaGraphics.cpp` anchors above without requiring proprietary bytes to be committed.
