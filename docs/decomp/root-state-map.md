# Provisional root game-state map

This file records root-state offsets whose meaning is supported by multiple
independent binary references. Names remain provisional until wider
class/constructor recovery establishes original types.

## Native boundary

The first native root-state boundary now lives in
`decomp/include/comet/root_state.hpp`. Fixed PS3 offsets are retained only as
reversing provenance. The eventual native game state must use ordinary typed
members and renderer/model resources rather than reproduce the original
multi-megabyte object ABI.

Structurally proven regions now include:

- `+0x2D2DC0`: arena model table, 40 slots at exact stride `0x90`;
- `+0x2D445C..+0x2D4488`: framebuffer-handle region recovered from
  `arenaGraphics.cpp`;
- `+0x2D448C..+0x2D44B8`: texture-handle region recovered from the same
  renderer setup;
- `+0x2D451C/+0x2D4520`: level-selection state;
- `+0x2D6438`: level-map runtime subobject.

## Root-state access miner

`tools/decomp_root_state_refs.py` now scans the exact PPU ELF directly for
the recurring PowerPC addressing shape:

```text
addis rX, root, 0x2d
lwz/stw/lbz/stb/lhz/sth/lfs/stfs/... value, low16(rX)
```

It recovers the title's OPD function starts, assigns each access to its owning
function, and records width plus read/write/address-taking sites. The scan is
intentionally conservative: it reports direct nearby uses of the derived
`root+0x2D0000` register and therefore provides a reproducible lower bound,
not a claim that every possible propagated pointer use has been found.

Running the current pass over the canonical NPEB00142 v1.00 ELF reproduces the
known `current_level_id` cross-reference cluster and exposes several
high-frequency gameplay fields immediately beside it:

| root offset | direct hits | functions | width | observed access mix |
| ---: | ---: | ---: | ---: | --- |
| `+0x2D451C` | 40 | 13 | 4 | 30 reads, 10 writes |
| `+0x2D4520` | 12 | 4 | 4 | 6 reads, 6 writes |
| `+0x2D4538` | 22 | 10 | 4 | 20 reads, 2 writes |
| `+0x2D4560` | 45 | 17 | 4 | 41 reads, 4 writes |
| `+0x2D4580` | 18 | 10 | 4 | 18 direct reads |
| `+0x2D4584` | 16 | 7 | 4 | 15 reads, 1 nearby write |
| `+0x2D4594` | 11 | 6 | 4 | 7 reads, 2 writes, 2 address takes |
| `+0x2D459C` | 15 | 8 | 4 | 14 float reads, 1 float write |

These adjacent fields deliberately remain unnamed. The access shapes establish
their widths and importance, but do not yet justify source-level meanings.

Two especially useful next anchors are:

- `+0x2D4538`: written at `0x000CA288` and `0x000CE52C`, then repeatedly
  used as an iteration/count bound in gameplay code;
- `+0x2D4560`: compared repeatedly with values 0, 1, 2 and 3 and written at
  `0x000D0A60`, `0x000E03D8`, `0x000E1408`, and `0x000E1B94`.
  This strongly establishes an enum/state-like role, but its semantic name is
  withheld until the transition paths are recovered.

## `+0x2D451C` — current level ID

Evidence:

- root-state construction at `0x000D5174` initializes this field to **1**;
- `0x000D5D5C` reads it twice: once to format `"%slevel%u.map"` and once as
  the level-ID argument to `0x000D91B0`;
- the access-miner pass finds 40 direct accesses across 13 OPD functions;
- multiple level-transition branches write a new value immediately before
  calling `0x000D5D5C`:
  - `0x000E0344` copies an ID from another level descriptor;
  - `0x000E0F1C` writes current+1;
  - `0x000E14AC`, `0x000E1B90`, `0x000E1DA4`, `0x000E1ED0`,
    `0x000E1F34`, and `0x000E1FA0` write explicit/derived IDs immediately
    before the load call.

This is sufficiently strong to use the native name **`current_level_id`**.

## `+0x2D4520` — adjacent requested/transition level ID

This field is repeatedly written alongside `current_level_id` during the same
transitions. For example `0x000E0F18` stores current+1 here immediately before
copying the same value into `+0x2D451C`.

The exact distinction between current, requested, next, and display level is
not yet fully recovered, so the legacy offset remains explicitly provisional
in the native header.

## `+0x2D6438` — level-map runtime state

Only two direct address-construction sites for this exact subobject were found
in the original targeted scan:

- `0x000D50BC` during root-state construction;
- `0x000D5DC4` immediately before calling the recovered map loader.

Constructor evidence at `0x000D50BC`:

1. zero exactly `0x88` bytes starting at `root + 0x2D6438`;
2. initialize the vector-like object at `root + 0x2D64C0` (`+0x88`) to empty;
3. initialize the vector-like object at `root + 0x2D64D0` (`+0x98`) to empty.

Loader `0x000D91B0` then fills exactly this layout and writes derived extent
values at subobject offsets `+0xA8/+0xAC`.

Therefore `root + 0x2D6438` is named **`level_map_state`** in the
decompilation track.

See `docs/decomp/level-map-format.md` for the recovered subobject and disk
layout.
