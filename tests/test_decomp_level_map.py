import importlib.util
import struct
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "decomp_level_map",
    ROOT / "tools" / "decomp_level_map.py",
)
mod = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = mod
assert SPEC.loader is not None
SPEC.loader.exec_module(mod)


def make_header(primary_count: int, secondary_count: int) -> bytes:
    header = bytearray(mod.HEADER_SIZE)
    header[0:4] = primary_count.to_bytes(4, "big")
    header[4:8] = secondary_count.to_bytes(4, "big")
    return bytes(header)


def make_primary(record_type: int, a: int = 0, b: int = 0) -> bytes:
    rec = bytearray(mod.PRIMARY_RECORD_SIZE)
    rec[0] = record_type
    rec[2] = a
    rec[3] = b
    return bytes(rec)


def make_secondary(order_value: float, selector: int, opcode: int) -> bytes:
    rec = bytearray(mod.SECONDARY_RECORD_SIZE)
    struct.pack_into(">f", rec, 0, order_value)
    rec[6] = selector
    rec[7] = opcode
    return bytes(rec)


class LevelMapParserTests(unittest.TestCase):
    def test_exact_section_size_and_extent_normalization(self):
        blob = (
            make_header(2, 2)
            + make_primary(0x09)
            + make_primary(0x0B, 16, 20)
            + make_secondary(1.0, 1, 21)
            + make_secondary(2.0, 12, 21)
        )
        parsed = mod.parse_level_map(blob)

        self.assertEqual(parsed.header.primary_record_count, 2)
        self.assertEqual(parsed.header.secondary_record_count, 2)
        self.assertTrue(parsed.has_extent_record)
        self.assertEqual(parsed.arena_extent, 20)
        self.assertEqual(parsed.primary_records[1].raw[2:4], b"\x14\x14")
        self.assertEqual(len(parsed.secondary_records), 1)
        self.assertAlmostEqual(parsed.secondary_records[0].order_value, 1.0)

    def test_default_extent_and_original_secondary_gate(self):
        blob = (
            make_header(1, 1)
            + make_primary(0x09)
            + make_secondary(3.0, 1, 21)
        )
        parsed = mod.parse_level_map(blob)

        self.assertFalse(parsed.has_extent_record)
        self.assertEqual(parsed.arena_extent, 24)
        self.assertEqual(parsed.raw_secondary_record_count, 1)
        self.assertEqual(parsed.secondary_records, ())

    def test_skip_secondary_mirrors_nonzero_fourth_argument(self):
        blob = (
            make_header(1, 1)
            + make_primary(0x0B, 16, 16)
            + make_secondary(3.0, 1, 21)
        )
        parsed = mod.parse_level_map(blob, skip_secondary=True)
        self.assertEqual(parsed.secondary_records, ())

    def test_selector_opcode_filter_matches_loader(self):
        kept = [
            make_secondary(1.0, 0, 20),
            make_secondary(1.0, 8, 30),
            make_secondary(1.0, 9, 0),
            make_secondary(1.0, 10, 28),
        ]
        rejected = [
            make_secondary(1.0, 11, 21),
            make_secondary(1.0, 0, 19),
            make_secondary(1.0, 0, 29),
            make_secondary(1.0, 0, 31),
            make_secondary(1.0, 9, 29),
            make_secondary(1.0, 10, 29),
        ]

        for rec in kept:
            self.assertTrue(mod.secondary_record_is_kept(rec))
        for rec in rejected:
            self.assertFalse(mod.secondary_record_is_kept(rec))

    def test_size_mismatch_is_rejected(self):
        with self.assertRaises(ValueError):
            mod.parse_level_map(make_header(1, 0))


if __name__ == "__main__":
    unittest.main()
