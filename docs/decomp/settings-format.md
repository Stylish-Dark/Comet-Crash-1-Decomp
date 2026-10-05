# Native settings.dat recovery

The original NPEB00142 v1.00 settings path is now recovered far enough to
replace PS3 memory-image persistence with an endian-safe native parser.

## File path and exact size

PPU function `0x000D5BD8` formats the literal path:

```text
%s/settings.dat
```

and opens it with mode `"wb"`. Function `0x000D5E18` opens the same path
with `"rb"`.

Both paths use the exact transfer size **3912 bytes**.

The loader accepts the read only when:

1. exactly 3912 bytes were returned; and
2. byte 0 equals **8**.

Otherwise it falls back to the recovered defaults and rewrites the file.

## Root-state ownership

The live settings blob begins at root offset **`+0x2D4598`**.

Direct address sites include:

- `0x000D5C64` — source for normal settings.dat save;
- `0x000D6078` — destination after a valid settings.dat read;
- `0x001392FC` / `0x0013953C` — PS3 savedata utility integration.

The latter path retains the surviving identifiers `sdu_auto_load`,
`sdu_auto_save`, and `NPEB00142-AUTO`. The native port does not need that
PS3 savedata layer; it can persist the same semantic settings state directly.

## Default 12-byte header

The default constructor at `0x000D5E7C..0x000D5EC4` yields:

| offset | default |
| ---: | ---: |
| `+0x00` | 8 |
| `+0x01` | 1 |
| `+0x02` | 60 |
| `+0x03` | 90 |
| `+0x04..+0x07` | 0 |
| `+0x08` | 1 |
| `+0x09` | 1 |
| `+0x0A` | 0 |
| `+0x0B` | 0 |

Only byte 0 currently has proven semantics: it is the **format version**.
The remaining header fields retain neutral names until their consumers prove
their meaning.

## Remaining 3900-byte matrix

The loop at `0x000D5EC8..0x000D5F44` initializes the rest of the file as a
fixed structure-of-arrays:

```text
3 banks
  x (3 lanes x 100 big-endian u32 values
     + 100 u8 flags)
```

Exact value offsets are:

```text
12 + bank*1200 + lane*400 + index*4
```

for `bank=0..2`, `lane=0..2`, `index=0..99`.

The three byte lanes begin at:

```text
3612 + bank*100 + index
```

Total:

```text
12 + 3*(3*100*4 + 100) = 3912 bytes
```

The nine u32 lanes are serialized in the PS3's big-endian byte order. The
native implementation therefore decodes/encodes explicitly instead of
reinterpret-casting a packed host struct.

## Shipped-data validation

The original title's factory
`USRDIR/data/user/settings.dat` independently matches all recovered
structural facts:

- exact size: 3912 bytes;
- header bytes:
  `08 01 3C 5A 00 00 00 00 01 01 00 00`;
- all nine 100-entry u32 lanes are zero;
- all three 100-byte lanes are zero.

No proprietary settings bytes are committed to the repository; this result is
recorded only as validation metadata.

## Native implementation

- `decomp/include/comet/settings.hpp`
- `decomp/src/settings.cpp`

The native API provides:

- `default_settings()`;
- strict `parse_settings()`;
- big-endian `serialize_settings()`;
- normal filesystem load/save wrappers.

This replaces both the raw PS3 memory-image assumption and the PS3 savedata
utility dependency while preserving the original on-disk format.
