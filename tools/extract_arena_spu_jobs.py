#!/usr/bin/env python3
"""Extract six raw Comet Crash SPURS job binaries from a user-owned PPU ELF.

These payloads have job headers, not ELF headers, so an embedded-ELF scan
does not discover them. Output is proprietary user-owned input and must stay
outside tracked source. The JSON report contains only addresses and hashes.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

from decomp_source_refs import load_segments

MODULE_TOC = 0x241DB8
# producer address, start-pointer TOC displacement, end-pointer displacement
JOB_POINTERS = (
    (0xFDC5C, -0x6330, -0x632C),
    (0xFDD5C, -0x6328, -0x6324),
    (0xFE1C0, -0x6320, -0x631C),
    (0xFE5D8, -0x6318, -0x6314),
    (0xFDFE0, -0x6310, -0x630C),
    (0xFDE54, -0x6300, -0x62FC),
)
MAX_JOB_SIZE = 256 * 1024


def read_range(blob: bytes, segments: list[tuple[int, int, int]],
               start: int, size: int) -> bytes:
    if size <= 0 or size > MAX_JOB_SIZE:
        raise ValueError("invalid SPU job range size")
    for va, offset, length in segments:
        if va <= start and start + size <= va + length:
            begin = offset + start - va
            if begin + size <= len(blob):
                return blob[begin:begin + size]
    raise ValueError(f"SPU job range 0x{start:X}+0x{size:X} is not file-backed")


def inspect_arena_jobs(blob: bytes) -> tuple[list[dict], list[bytes]]:
    if len(blob) < 64:
        raise ValueError("truncated ELF header")
    phoff = struct.unpack_from(">Q", blob, 0x20)[0]
    phentsize, phnum = struct.unpack_from(">HH", blob, 0x36)
    if phentsize < 56 or phoff + phentsize * phnum > len(blob):
        raise ValueError("invalid ELF program header table")
    segments = load_segments(blob)
    report, payloads = [], []
    for producer, start_offset, end_offset in JOB_POINTERS:
        start = struct.unpack(">I", read_range(blob, segments, MODULE_TOC + start_offset, 4))[0]
        end = struct.unpack(">I", read_range(blob, segments, MODULE_TOC + end_offset, 4))[0]
        if start & 15 or end & 3 or end <= start:
            raise ValueError(f"invalid SPU job pointers for producer 0x{producer:X}")
        data = read_range(blob, segments, start, end - start)
        # All six original job headers select register 80 mode at +0x10/+0x18,
        # followed by direct SPU branches at +0x14/+0x1C.
        if len(data) < 32 or struct.unpack_from(">I", data, 16)[0] != 0x4400A850:
            raise ValueError(f"SPU job header mismatch for producer 0x{producer:X}")
        for offset in (20, 28):
            if struct.unpack_from(">I", data, offset)[0] >> 23 != 0x64:
                raise ValueError("SPU job header branch mismatch")
        report.append({
            "producer": f"0x{producer:08X}",
            "start_toc_slot": f"0x{MODULE_TOC + start_offset:08X}",
            "end_toc_slot": f"0x{MODULE_TOC + end_offset:08X}",
            "start": f"0x{start:08X}", "end_exclusive": f"0x{end:08X}",
            "size": len(data), "sha256": hashlib.sha256(data).hexdigest(),
            "filename": f"arena-job-{producer:08x}.bin",
        })
        payloads.append(data)
    return report, payloads


def extract_arena_jobs(elf: Path, output: Path) -> list[dict]:
    # Validate all ranges before writing any output.
    report, payloads = inspect_arena_jobs(elf.read_bytes())
    output.mkdir(parents=True, exist_ok=True)
    for item, payload in zip(report, payloads):
        (output / item["filename"]).write_bytes(payload)
    (output / "arena-jobs.json").write_text(json.dumps(report, indent=2) + "\n")
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    for item in extract_arena_jobs(args.elf, args.output):
        print(f'{item["producer"]}: {item["start"]}..{item["end_exclusive"]} '
              f'{item["size"]} bytes sha256={item["sha256"]}')
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
