import importlib.util
import struct
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
SPEC = importlib.util.spec_from_file_location(
    "decomp_model_disasm", ROOT / "tools" / "decomp_model_disasm.py")
mod = importlib.util.module_from_spec(SPEC)
assert SPEC.loader
SPEC.loader.exec_module(mod)


def synthetic_elf() -> bytes:
    blob = bytearray(0x200)
    blob[:6] = b"\x7fELF\x02\x02"
    struct.pack_into(">Q", blob, 0x20, 0x40)  # ELF program headers
    struct.pack_into(">H", blob, 0x36, 56)
    struct.pack_into(">H", blob, 0x38, 1)
    struct.pack_into(">I", blob, 0x40, 1)  # PT_LOAD
    struct.pack_into(">Q", blob, 0x48, 0x100)
    struct.pack_into(">Q", blob, 0x50, 0x105300)
    struct.pack_into(">Q", blob, 0x60, 0x20)
    blob[0x108:0x110] = bytes.fromhex("386000014e800020") # li r3,1; blr
    return bytes(blob)


class ModelDisassemblyTests(unittest.TestCase):
    def test_extracts_exact_virtual_range(self):
        data = synthetic_elf()
        self.assertEqual(mod.read_virtual_range(data, 0x105308, 0x105310),
                         bytes.fromhex("386000014e800020"))
        ins = mod.inspect(data, 0x105308, 0x105310)
        self.assertEqual(len(ins), 2)
        self.assertEqual(ins[0]["address"], "0x00105308")
        self.assertEqual(ins[-1]["address"], "0x0010530C")

    def test_rejects_unbacked_truncated_and_misaligned_ranges(self):
        data = synthetic_elf()
        for start, end in ((0x105307, 0x105310),
                           (0x105308, 0x105311),
                           (0x105320, 0x105324),
                           (0x105310, 0x105308)):
            with self.subTest(start=start, end=end):
                with self.assertRaises(ValueError):
                    mod.read_virtual_range(data, start, end)
        with self.assertRaises(ValueError):
            mod.read_virtual_range(data[:0x104], 0x105308, 0x105310)


if __name__ == "__main__":
    unittest.main()
