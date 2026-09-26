# Provisional root game-state map

This file records root-state offsets whose meaning is supported by multiple independent binary references. Names remain provisional until wider class/constructor recovery establishes original types.

## `+0x2D451C` — current level ID

Evidence:

- root-state construction at `0x000D5174` initializes this field to **1**;
- `0x000D5D5C` reads it twice: once to format `"%slevel%u.map"` and once as the level-ID argument to `0x000D91B0`;
- the executable contains roughly 60 direct accesses using the same `addis base,0x2D ; ... 0x451C` addressing pattern;
- multiple level-transition branches write a new value immediately before calling `0x000D5D5C`:
  - `0x000E0344` copies an ID from another level descriptor;
  - `0x000E0F1C` writes current+1;
  - `0x000E14AC`, `0x000E1B90`, `0x000E1DA4`, `0x000E1ED0`, `0x000E1F34`, and `0x000E1FA0` write explicit/derived IDs immediately before the load call.

This is sufficiently strong to use the native name **`current_level_id`**.

## `+0x2D4520` — adjacent requested/transition level ID

This field is repeatedly written alongside `current_level_id` during the same transitions. For example `0x000E0F18` stores current+1 here immediately before copying the same value into `+0x2D451C`.

The exact distinction between current, requested, next, and display level is not yet fully recovered, so keep this field provisional.

## `+0x2D6438` — level-map runtime state

Only two direct address-construction sites for this exact subobject were found in the current code scan:

- `0x000D50BC` during root-state construction;
- `0x000D5DC4` immediately before calling the recovered map loader.

Constructor evidence at `0x000D50BC`:

1. zero exactly `0x88` bytes starting at `root + 0x2D6438`;
2. initialize the vector-like object at `root + 0x2D64C0` (`+0x88`) to empty;
3. initialize the vector-like object at `root + 0x2D64D0` (`+0x98`) to empty.

Loader `0x000D91B0` then fills exactly this layout and writes derived extent values at subobject offsets `+0xA8/+0xAC`.

Therefore `root + 0x2D6438` is now named **`level_map_state`** in the decompilation track.

See `docs/decomp/level-map-format.md` for the recovered subobject and disk layout.
