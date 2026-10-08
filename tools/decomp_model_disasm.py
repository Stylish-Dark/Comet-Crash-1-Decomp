#!/usr/bin/env python3
"""Inspect the exact-title PPU model loader without storing proprietary bytes.

Reads the user's own ELF64 big-endian EBOOT and disassembles a bounded virtual
address range. Output is an address-anchored JSON or text report suitable for
tracking OBJ face parsing, source-index normalization, and vertex collapse.

This is an evidence capture tool, not a symbolic decompiler. No assumptions
about the original signed-index policy are made.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

from capstone import CS_ARCH_PPC, CS_MODE_64, CS_MODE_BIG_ENDIAN, Cs

from decomp_source_refs import load_segments

MODEL_START = 0x00105308
MODEL_END_EXCLUSIVE = 0x0010B538
DEFAULT_MAX_BYTES = 0x20000


def read_virtual_range(blob: bytes, start: int, end: int) -> bytes:
    """Read a range wholly backed by a single file-backed PT_LOAD segment."""
    if start < 0 or end <= start or (start | end) & 3:
        raise ValueError("expected aligned, increasing virtual addresses")
    segments = load_segments(blob)
    for va, offset, size in segments:
        if va <= start and end <= va + size:
            first = offset + start - va
            last = first + end - start
            if last > len(blob):
                raise ValueError("PT_LOAD points beyond the ELF file")
            return blob[first:last]
    raise ValueError("range is not wholly contained in a file-backed PT_LOAD segment")


def inspect(blob: bytes, start: int, end: int) -> list[dict]:
    code = read_virtual_range(blob, start, end)
    md = Cs(CS_ARCH_PPC, CS_MODE_64 | CS_MODE_BIG_ENDIAN)
    rows = [
        {
            "address": f"0x{ins.address:08X}",
            "mnemonic": ins.mnemonic,
            "operands": ins.op_str,
        }
        for ins in md.disasm(code, start)
    ]
    if len(rows) != (end - start) // 4:
        raise ValueError("disassembler could not decode every instruction in range")
    return rows


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("elf", type=Path, help="user-owned decrypted PPU ELF")
    ap.add_argument("--start", type=lambda s: int(s, 0), default=MODEL_START)
    ap.add_argument("--end", type=lambda s: int(s, 0), default=MODEL_END_EXCLUSIVE)
    ap.add_argument("--max-bytes", type=lambda s: int(s, 0), default=DEFAULT_MAX_BYTES)
    ap.add_argument("--format", choices=("text", "json"), default="text")
    ap.add_argument("--output", type=Path, help="write locally; no ELF data committed")
    args = ap.parse_args()
    if args.end <= args.start or args.end - args.start > args.max_bytes:
        ap.error("range must be nonempty and within --max-bytes")
    rows = inspect(args.elf.read_bytes(), args.start, args.end)
    if args.format == "json":
        report = json.dumps({
            "start": f"0x{args.start:08X}",
            "end_exclusive": f"0x{args.end:08X}",
            "instructions": rows,
        }, indent=2) + "\n"
    else:
        report = "".join(
            f"{row['address']}  {row['mnemonic']:<10} {row['operands']}\n"
            for row in rows
        )
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(report, encoding="utf-8")
    else:
        print(report, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
