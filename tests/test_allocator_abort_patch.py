import struct
import sys
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
import patch_allocator_abort as p

class AllocatorAbortPatchTests(unittest.TestCase):
    def fixture(self) -> bytes:
        data=bytearray(0x300)
        data[:6]=b"\x7fELF\x02\x02"
        struct.pack_into(">Q",data,32,0x40)
        struct.pack_into(">H",data,54,56)
        struct.pack_into(">H",data,56,1)
        # PT_LOAD maps file 0x100 -> guest 0x001A4E00 for 0x100 bytes.
        struct.pack_into(">IIQQQQQQ",data,0x40,1,5,0x100,0x001A4E00,0,0x100,0x100,0x1000)
        off=0x100+(p.PATCH_VADDR-0x001A4E00)
        data[off:off+4]=p.EXPECTED
        return bytes(data)

    def test_exact_call_is_replaced_with_nop(self):
        src=self.fixture()
        out=p.patch_bytes(src,require_reference_hash=False)
        off=p.file_offset_for_vaddr(out,p.PATCH_VADDR)
        self.assertEqual(out[off:off+4],p.REPLACEMENT)

    def test_unexpected_instruction_fails(self):
        src=bytearray(self.fixture())
        off=p.file_offset_for_vaddr(src,p.PATCH_VADDR)
        src[off:off+4]=b"\x00\x00\x00\x00"
        with self.assertRaisesRegex(ValueError,"unexpected instruction"):
            p.patch_bytes(bytes(src),require_reference_hash=False)

if __name__=="__main__":
    unittest.main()
