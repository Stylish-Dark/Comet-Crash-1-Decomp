import hashlib
import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from extract_arena_spu_jobs import JOB_POINTERS, MODULE_TOC, extract_arena_jobs, inspect_arena_jobs


def fixture():
    base, file_offset, length = 0x1C0000, 0x100, 0x90000
    blob = bytearray(file_offset + length)
    blob[:6] = b"\x7fELF\x02\x02"
    struct.pack_into(">Q", blob, 0x20, 0x40)
    struct.pack_into(">HH", blob, 0x36, 56, 1)
    struct.pack_into(">I", blob, 0x40, 1)
    struct.pack_into(">Q", blob, 0x48, file_offset)
    struct.pack_into(">Q", blob, 0x50, base)
    struct.pack_into(">Q", blob, 0x60, length)
    for index, (_, lo, hi) in enumerate(JOB_POINTERS):
        start = base + 0x200 + index * 32
        struct.pack_into(">I", blob, file_offset + MODULE_TOC + lo - base, start)
        struct.pack_into(">I", blob, file_offset + MODULE_TOC + hi - base, start + 32)
        struct.pack_into(">IIII", blob, file_offset + start - base + 16,
                         0x4400A850, 0x32000080, 0x4400A850, 0x32000080)
    return blob


class ArenaJobsTests(unittest.TestCase):
    def test_six_non_elf_job_ranges(self):
        report, data = inspect_arena_jobs(bytes(fixture()))
        self.assertEqual(len(report), 6)
        self.assertEqual(len(data), 6)
        self.assertEqual(report[-1]["producer"], "0x000FDE54")
        self.assertTrue(all(x["size"] == 32 for x in report))
        self.assertEqual(report[0]["sha256"], hashlib.sha256(data[0]).hexdigest())

    def test_validates_all_jobs_before_output(self):
        blob = fixture()
        blob[0x100 + 0x200 + 5 * 32 + 16] = 0
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "input.elf"
            source.write_bytes(blob)
            output = Path(directory) / "jobs"
            with self.assertRaises(ValueError):
                extract_arena_jobs(source, output)
            self.assertFalse(output.exists())

    def test_rejects_range_beyond_segment(self):
        blob = fixture()
        struct.pack_into(">I", blob, 0x100 + MODULE_TOC + JOB_POINTERS[0][1] - 0x1C0000, 0x250000)
        with self.assertRaises(ValueError):
            inspect_arena_jobs(blob)

    def test_rejects_invalid_branch_header(self):
        blob = fixture()
        blob[0x100 + 0x200 + 20] = 0
        with self.assertRaises(ValueError):
            inspect_arena_jobs(blob)

    def test_short_elf(self):
        with self.assertRaises(ValueError):
            inspect_arena_jobs(b"\x7fELF\x02\x02")


if __name__ == "__main__":
    unittest.main()
