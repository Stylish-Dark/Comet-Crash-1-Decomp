import importlib.util
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "decomp_root_state_refs.py"

spec = importlib.util.spec_from_file_location("root_refs", TOOL)
root_refs = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = root_refs
spec.loader.exec_module(root_refs)


def dform(opcode, rt, ra, imm):
    return (opcode << 26) | (rt << 21) | (ra << 16) | (imm & 0xFFFF)


class RootStateReferenceMinerTests(unittest.TestCase):
    def test_recovers_read_write_and_address_evidence(self):
        words = [
            (0x1000, dform(15, 9, 3, 0x2D)),
            (0x1004, dform(32, 6, 9, 0x451C)),
            (0x1008, dform(36, 7, 9, 0x4520)),
            (0x100C, dform(14, 8, 9, 0x6438)),
        ]
        hits = root_refs.scan_words(words, [0x1000])
        self.assertEqual(
            [(h["offset"], h["kind"], h["width"]) for h in hits],
            [
                (0x2D451C, "read", 4),
                (0x2D4520, "write", 4),
                (0x2D6438, "address", 0),
            ],
        )

    def test_ignores_other_high_halves(self):
        words = [
            (0x2000, dform(15, 9, 3, 0x2C)),
            (0x2004, dform(32, 6, 9, 0x451C)),
        ]
        self.assertEqual(root_refs.scan_words(words, [0x2000]), [])

    def test_summary_groups_functions_and_access_kinds(self):
        words = [
            (0x3000, dform(15, 9, 3, 0x2D)),
            (0x3004, dform(32, 6, 9, 0x4560)),
            (0x4000, dform(15, 10, 4, 0x2D)),
            (0x4004, dform(36, 6, 10, 0x4560)),
        ]
        rows = root_refs.summarize(
            root_refs.scan_words(words, [0x3000, 0x4000])
        )
        self.assertEqual(rows[0]["offset"], "0x002D4560")
        self.assertEqual(rows[0]["access_count"], 2)
        self.assertEqual(rows[0]["function_count"], 2)
        self.assertEqual(rows[0]["read_count"], 1)
        self.assertEqual(rows[0]["write_count"], 1)
        self.assertEqual(rows[0]["width_bytes"], [4])


if __name__ == "__main__":
    unittest.main()
