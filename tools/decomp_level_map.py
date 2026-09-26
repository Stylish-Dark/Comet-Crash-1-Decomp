#!/usr/bin/env python3
"""Parse Comet Crash NPEB00142 v1.00 level*.map blobs.

This is a reverse-engineering aid for the native decompilation track.  It
models only structure proven by the loader at PPU 0x000D91B0:

    0x88-byte header
    header[0] * 0x38-byte primary records
    header[1] * 0x18-byte secondary/timeline records

The original loader normalizes type-0x0B primary records, derives an arena
extent from them (default 24 when absent), and conditionally keeps secondary
records through a small selector/opcode filter.

No proprietary data is embedded in this module.
"""
from __future__ import annotations

import argparse
import dataclasses
import json
import struct
from pathlib import Path

HEADER_SIZE = 0x88
PRIMARY_RECORD_SIZE = 0x38
SECONDARY_RECORD_SIZE = 0x18
DEFAULT_ARENA_EXTENT = 24
EXTENT_RECORD_TYPE = 0x0B


def be_u32(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset:offset + 4], "big")


def be_f32(data: bytes, offset: int) -> float:
    return struct.unpack_from(">f", data, offset)[0]


@dataclasses.dataclass(frozen=True)
class LevelMapHeader:
    raw: bytes

    @property
    def primary_record_count(self) -> int:
        return be_u32(self.raw, 0x00)

    @property
    def secondary_record_count(self) -> int:
        return be_u32(self.raw, 0x04)

    def u32(self, index: int) -> int:
        if not 0 <= index < HEADER_SIZE // 4:
            raise IndexError(index)
        return be_u32(self.raw, index * 4)


@dataclasses.dataclass(frozen=True)
class PrimaryRecord:
    raw: bytes

    @property
    def record_type(self) -> int:
        return self.raw[0]


@dataclasses.dataclass(frozen=True)
class SecondaryRecord:
    raw: bytes

    @property
    def order_value(self) -> float:
        """First big-endian float.

        Across every currently known embedded/file-backed map this value is
        monotonically nondecreasing within the secondary section.  It is kept
        as a provisional ordering/time value until downstream consumers prove
        the exact gameplay meaning.
        """
        return be_f32(self.raw, 0)

    @property
    def selector(self) -> int:
        return self.raw[6]

    @property
    def opcode(self) -> int:
        return self.raw[7]


@dataclasses.dataclass(frozen=True)
class ParsedLevelMap:
    header: LevelMapHeader
    primary_records: tuple[PrimaryRecord, ...]
    secondary_records: tuple[SecondaryRecord, ...]
    has_extent_record: bool
    arena_extent: int
    raw_secondary_record_count: int


def secondary_record_is_kept(record: bytes) -> bool:
    """Exact keep/reject predicate from PPU 0x000D947C..0x000D94B8."""
    if len(record) != SECONDARY_RECORD_SIZE:
        raise ValueError("secondary record must be exactly 0x18 bytes")

    selector = record[6]
    opcode = record[7]

    if selector > 10:
        return False

    if selector in (9, 10):
        return opcode != 29

    return 20 <= opcode <= 30 and opcode != 29


def expected_file_size(header: LevelMapHeader) -> int:
    return (
        HEADER_SIZE
        + header.primary_record_count * PRIMARY_RECORD_SIZE
        + header.secondary_record_count * SECONDARY_RECORD_SIZE
    )


def parse_level_map(data: bytes, *, skip_secondary: bool = False) -> ParsedLevelMap:
    if len(data) < HEADER_SIZE:
        raise ValueError(f"truncated level-map header: {len(data)} < {HEADER_SIZE}")

    header = LevelMapHeader(data[:HEADER_SIZE])
    expected = expected_file_size(header)
    if len(data) != expected:
        raise ValueError(
            "level-map size mismatch: "
            f"got {len(data)}, expected {expected} "
            f"(primary={header.primary_record_count}, "
            f"secondary={header.secondary_record_count})"
        )

    pos = HEADER_SIZE
    primary: list[PrimaryRecord] = []
    has_extent = False
    arena_extent = DEFAULT_ARENA_EXTENT

    for _ in range(header.primary_record_count):
        raw = bytearray(data[pos:pos + PRIMARY_RECORD_SIZE])
        pos += PRIMARY_RECORD_SIZE

        if raw[0] == EXTENT_RECORD_TYPE:
            has_extent = True
            # Exact loader behaviour: force the two byte-sized extents equal to
            # their maximum and copy that same derived value to both runtime
            # extent fields at state + 0xA8/+0xAC.
            arena_extent = max(raw[2], raw[3])
            raw[2] = arena_extent
            raw[3] = arena_extent

        primary.append(PrimaryRecord(bytes(raw)))

    secondary: list[SecondaryRecord] = []

    # 0x000D91B0 only consumes the second section when an extent marker was
    # encountered and the caller's final byte/boolean argument is zero.
    if has_extent and not skip_secondary:
        for _ in range(header.secondary_record_count):
            raw = data[pos:pos + SECONDARY_RECORD_SIZE]
            pos += SECONDARY_RECORD_SIZE
            if secondary_record_is_kept(raw):
                secondary.append(SecondaryRecord(raw))

    return ParsedLevelMap(
        header=header,
        primary_records=tuple(primary),
        secondary_records=tuple(secondary),
        has_extent_record=has_extent,
        arena_extent=arena_extent,
        raw_secondary_record_count=header.secondary_record_count,
    )


def _summary(parsed: ParsedLevelMap) -> dict:
    type_hist: dict[str, int] = {}
    for rec in parsed.primary_records:
        key = f"0x{rec.record_type:02X}"
        type_hist[key] = type_hist.get(key, 0) + 1

    secondary_hist: dict[str, int] = {}
    for rec in parsed.secondary_records:
        key = f"{rec.selector}:{rec.opcode}"
        secondary_hist[key] = secondary_hist.get(key, 0) + 1

    order_values = [rec.order_value for rec in parsed.secondary_records]
    return {
        "primary_record_count": parsed.header.primary_record_count,
        "secondary_record_count_on_disk": parsed.raw_secondary_record_count,
        "secondary_record_count_kept": len(parsed.secondary_records),
        "has_extent_record": parsed.has_extent_record,
        "arena_extent": parsed.arena_extent,
        "primary_type_histogram": type_hist,
        "secondary_selector_opcode_histogram": secondary_hist,
        "secondary_order_nondecreasing": all(
            order_values[i] <= order_values[i + 1]
            for i in range(len(order_values) - 1)
        ),
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("map", type=Path)
    ap.add_argument(
        "--skip-secondary",
        action="store_true",
        help="mirror 0x000D91B0's nonzero final argument",
    )
    args = ap.parse_args()

    parsed = parse_level_map(
        args.map.read_bytes(),
        skip_secondary=args.skip_secondary,
    )
    print(json.dumps(_summary(parsed), indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
