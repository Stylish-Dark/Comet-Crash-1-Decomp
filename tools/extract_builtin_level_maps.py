#!/usr/bin/env python3
"""Extract Comet Crash's 29 EBOOT-embedded level-map blobs.

PPU 0x000D91B0 indexes a fixed 29-entry (pointer,size) table at virtual address
0x00232140 for level IDs 0..28.  The native port should not carry those maps as
opaque executable data; this tool recovers them from the user's own decrypted
NPEB00142 v1.00 EBOOT and writes ordinary level0.map .. level28.map files.

The output is user-owned game data and must not be committed to the repository.
"""
from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path

from decomp_level_map import parse_level_map
from decomp_source_refs import load_segments, va_to_offset

BUILTIN_TABLE_VA = 0x00232140
BUILTIN_LEVEL_COUNT = 29


def extract_builtin_maps(elf_path: Path, output_dir: Path) -> list[Path]:
    blob = elf_path.read_bytes()
    segments = load_segments(blob)

    table_off = va_to_offset(segments, BUILTIN_TABLE_VA)
    if table_off is None:
        raise ValueError(
            f"builtin map table VA 0x{BUILTIN_TABLE_VA:08X} is not file-backed"
        )

    output_dir.mkdir(parents=True, exist_ok=True)
    written: list[Path] = []

    for level_id in range(BUILTIN_LEVEL_COUNT):
        entry_off = table_off + level_id * 8
        data_va, size = struct.unpack_from(">II", blob, entry_off)

        data_off = va_to_offset(segments, data_va)
        if data_off is None or data_off + size > len(blob):
            raise ValueError(
                f"level {level_id}: invalid embedded range "
                f"VA=0x{data_va:08X} size=0x{size:X}"
            )

        payload = blob[data_off:data_off + size]

        # Structural validation is intentionally performed before writing.
        # It catches an incorrect title/version/table address without requiring
        # proprietary reference hashes in the repository.
        parse_level_map(payload)

        out = output_dir / f"level{level_id}.map"
        out.write_bytes(payload)
        written.append(out)

        digest = hashlib.sha256(payload).hexdigest()
        print(
            f"level {level_id:2d}: {size:6d} bytes "
            f"VA=0x{data_va:08X} sha256={digest}"
        )

    return written


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("elf", type=Path, help="decrypted NPEB00142 v1.00 EBOOT.ELF")
    ap.add_argument("output", type=Path)
    args = ap.parse_args()

    files = extract_builtin_maps(args.elf, args.output)
    print(f"wrote {len(files)} embedded maps to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
