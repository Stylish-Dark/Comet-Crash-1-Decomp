# Level-map format and loader recovery

This document records the current decompilation of **PPU 0x000D91B0..0x000DA254** in NPEB00142 v1.00.

The conclusions below come from exact PPC disassembly plus structural validation against all currently available `level*.map` files and all 29 level blobs embedded in the reference executable. Names such as "primary", "secondary", "extent" and "order value" remain provisional where the binary proves structure but not the original source identifier.

## Function signature

The call sites support the following semantic signature:

```cpp
bool load_level_map(
    LevelMapState* state,
    uint32_t level_id,
    const char* path,
    bool skip_secondary);
```

Evidence:

- `0x000D5D5C` passes `game_state + 0x2D6438`, the current level ID, a formatted `level%u.map` path, and its caller-controlled final byte argument.
- The independent call at `0x0011EF88` passes another level-map state object, a level ID, a formatted path, and literal `1` for the final argument.
- The final argument is tested only as zero/nonzero at `0x000D942C`; nonzero skips the entire second-record section.

## Source selection

The original function supports two input sources.

For **level IDs 0..28**, it does not open a file. It indexes a 29-entry table at virtual address **0x00232140**. Each entry is:

```text
u32 data_va
u32 byte_size
```

The loader then consumes the embedded bytes exactly as though they were a file.

For **level IDs > 28**, it opens the supplied path with `"rb"` and reads the same binary format.

The native rewrite removes this architectural split. `tools/extract_builtin_level_maps.py` extracts the 29 embedded blobs from the user's own decrypted EBOOT into ordinary `level0.map .. level28.map` files. Native code can therefore use one file-backed loader for every level without carrying opaque executable data forward.

## Exact disk layout

Every known blob satisfies:

```text
file_size =
    0x88
    + primary_record_count   * 0x38
    + secondary_record_count * 0x18
```

Layout:

| Offset | Size | Meaning |
| ---: | ---: | --- |
| `0x00` | `4` | big-endian primary-record count |
| `0x04` | `4` | big-endian secondary-record count |
| `0x08` | `0x80` | remaining header payload; semantics still being recovered |
| `0x88` | `count0 * 0x38` | primary records |
| after primary | `count1 * 0x18` | secondary records |

The loader copies the entire 0x88-byte header directly into the start of its runtime state object.

## Runtime state layout proven by the loader

The PS3 object begins with the disk header and immediately follows it with two vector-like containers:

```text
+0x000 .. +0x087  raw 0x88-byte disk header
+0x088             std::vector-like container of 0x38-byte primary records
+0x098             std::vector-like container of 0x18-byte secondary records
+0x0A8             derived extent field A
+0x0AC             derived extent field B
```

Both vector objects use the PS3 STL layout in which the begin/end/capacity pointers are at container offsets `+4/+8/+0xC`.

The native rewrite deliberately does **not** preserve this STL/ABI layout. It represents the two record sets as normal native vectors.

## Primary records (0x38 bytes)

Every primary record is copied verbatim into the first runtime vector, with one proven exception.

When `record[0] == 0x0B`:

1. take byte fields `record[2]` and `record[3]`;
2. compute their maximum;
3. overwrite **both** bytes with that maximum;
4. write the same value to runtime state fields `+0xA8` and `+0xAC`;
5. append the normalized 0x38-byte record normally.

Across all currently known maps, there is at most one 0x0B record. Its observed extent is 16 or 20. If no 0x0B record exists, the runtime writes **24** to both derived extent fields.

This strongly suggests a square arena/grid extent, but the source name remains provisional until downstream users of `+0xA8/+0xAC` are recovered.

Observed first-byte primary record types across the current corpus are:

```text
0x09 0x0A 0x0B 0x14 0x15 0x16 0x17 0x18 0x19 0x1A 0x1C 0x1D
```

No semantic enum names have been assigned yet.

## Secondary records (0x18 bytes)

The first four bytes decode as a big-endian IEEE-754 float. Within every known secondary section, this float is monotonically nondecreasing. That is strong evidence for a time/order key, but the exact gameplay name remains provisional.

The loader consumes the secondary section only when:

- a primary type-0x0B extent record was encountered; and
- `skip_secondary == false`.

Each 0x18-byte record then passes the exact filter below:

```cpp
selector = record[6];
opcode   = record[7];

if (selector > 10)
    reject;

if (selector == 9 || selector == 10)
    keep if opcode != 29;
else
    keep if opcode is 20..30 inclusive and opcode != 29;
```

Accepted records are appended unchanged to the second runtime vector.

In the known corpus, bytes `9..23` are always zero and byte `8` is nonzero in only four records. This is evidence about current data, not permission to shrink the record: the on-disk stride remains exactly 0x18.

## Built-in level metadata

All 29 embedded entries at `0x00232140` validate against the exact size equation:

| ID | bytes | primary | secondary | extent marker | derived extent | secondary kept |
| ---: | ---: | ---: | ---: | :---: | ---: | ---: |
| 0 | 8392 | 147 | 1 | yes | 16 | 1 |
| 1 | 5664 | 91 | 18 | yes | 16 | 18 |
| 2 | 4792 | 75 | 19 | yes | 16 | 19 |
| 3 | 6256 | 102 | 17 | yes | 16 | 17 |
| 4 | 15368 | 272 | 0 | yes | 20 | 0 |
| 5 | 6112 | 72 | 81 | yes | 16 | 81 |
| 6 | 8408 | 131 | 39 | yes | 16 | 39 |
| 7 | 10272 | 181 | 0 | yes | 20 | 0 |
| 8 | 8696 | 146 | 16 | yes | 16 | 16 |
| 9 | 6224 | 89 | 46 | yes | 16 | 46 |
| 10 | 9896 | 143 | 73 | yes | 16 | 73 |
| 11 | 7808 | 137 | 0 | yes | 16 | 0 |
| 12 | 7480 | 93 | 89 | yes | 16 | 89 |
| 13 | 16376 | 290 | 0 | yes | 20 | 0 |
| 14 | 9936 | 142 | 77 | yes | 20 | 77 |
| 15 | 9408 | 142 | 55 | yes | 20 | 55 |
| 16 | 12408 | 160 | 138 | yes | 20 | 138 |
| 17 | 13008 | 181 | 114 | yes | 20 | 114 |
| 18 | 17328 | 307 | 0 | yes | 20 | 0 |
| 19 | 11824 | 171 | 88 | yes | 20 | 88 |
| 20 | 30248 | 281 | 599 | no | 24 | 0 |
| 21 | 14192 | 251 | 0 | yes | 20 | 0 |
| 22 | 8680 | 132 | 48 | yes | 20 | 48 |
| 23 | 14000 | 176 | 167 | no | 24 | 0 |
| 24 | 10160 | 179 | 0 | yes | 16 | 0 |
| 25 | 13408 | 147 | 210 | yes | 20 | 210 |
| 26 | 12432 | 127 | 216 | yes | 20 | 216 |
| 27 | 17368 | 291 | 39 | yes | 20 | 39 |
| 28 | 13432 | 120 | 274 | yes | 20 | 274 |

IDs 20 and 23 are particularly useful confirmation of the control flow: they contain secondary bytes on disk but no 0x0B primary marker, so the original loader intentionally leaves their secondary runtime vector empty.

## Native implementation

- `decomp/include/comet/level_map.hpp`
- `decomp/src/level_map.cpp`
- `tools/decomp_level_map.py`
- `tools/extract_builtin_level_maps.py`

The C++ parser is the semantic replacement for the parsing portion of `0x000D91B0`. It uses host-native containers and endian conversion rather than preserving PS3 register/STL ABI details.
