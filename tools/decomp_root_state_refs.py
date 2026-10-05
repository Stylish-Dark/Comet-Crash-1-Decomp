#!/usr/bin/env python3
"""Mine root-game-state accesses from an exact Comet Crash PPU ELF.

The title commonly addresses the monolithic root object with
`addis rX, root, 0x2d` followed by a D/DS-form load, store, or `addi`.
This decoder records only metadata; it never emits title bytes.
"""
from __future__ import annotations

import argparse
import bisect
import json
import struct
from collections import Counter, defaultdict
from pathlib import Path

PT_LOAD = 1
PF_X = 1
PF_W = 2
ROOT_HI16 = 0x2D

D_FORM_MEMORY = {
    32: ("read", 4), 34: ("read", 1),
    36: ("write", 4), 38: ("write", 1),
    40: ("read", 2), 42: ("read", 2), 44: ("write", 2),
    48: ("read", 4), 50: ("read", 8),
    52: ("write", 4), 54: ("write", 8),
}


def _u16(b, o): return struct.unpack_from(">H", b, o)[0]
def _u32(b, o): return struct.unpack_from(">I", b, o)[0]
def _u64(b, o): return struct.unpack_from(">Q", b, o)[0]
def _s16(v): return v - 0x10000 if v & 0x8000 else v


def load_segments(blob):
    if blob[:4] != b"\x7fELF" or blob[4] != 2 or blob[5] != 2:
        raise ValueError("expected ELF64 big-endian input")
    phoff = _u64(blob, 0x20)
    entsize = _u16(blob, 0x36)
    count = _u16(blob, 0x38)
    out = []
    for i in range(count):
        o = phoff + i * entsize
        if _u32(blob, o) != PT_LOAD:
            continue
        out.append({
            "flags": _u32(blob, o + 4),
            "file_offset": _u64(blob, o + 8),
            "vaddr": _u64(blob, o + 16),
            "filesz": _u64(blob, o + 32),
        })
    return out


def va_to_offset(segments, va):
    for s in segments:
        if s["vaddr"] <= va < s["vaddr"] + s["filesz"]:
            return s["file_offset"] + va - s["vaddr"]
    return None


def recover_opd_functions(blob, segments):
    def is_exec(va):
        return any(
            s["flags"] & PF_X and s["vaddr"] <= va < s["vaddr"] + s["filesz"]
            for s in segments
        )

    candidates = []
    for seg in segments:
        if not (seg["flags"] & PF_W):
            continue
        lo = (seg["file_offset"] + 7) & ~7
        hi = min(len(blob), seg["file_offset"] + seg["filesz"])
        run = []
        for o in range(lo, hi - 7, 8):
            code, toc = struct.unpack_from(">II", blob, o)
            valid = is_exec(code) and va_to_offset(segments, toc) is not None
            if valid:
                run.append((code, toc))
            else:
                if len(run) >= 32:
                    candidates.append(run)
                run = []
        if len(run) >= 32:
            candidates.append(run)
    if not candidates:
        raise ValueError("no PS3 OPD descriptor run found")
    by_start = {}
    for start, toc in max(candidates, key=len):
        by_start.setdefault(start, toc)
    return sorted(by_start)


def _decode_memory(word):
    opcode = word >> 26
    if opcode in D_FORM_MEMORY:
        kind, width = D_FORM_MEMORY[opcode]
        return (word >> 16) & 31, kind, width, _s16(word & 0xFFFF)
    if opcode in (58, 62):  # ld/std DS form
        ra = (word >> 16) & 31
        ds = word & 0xFFFC
        disp = ds - 0x10000 if ds & 0x8000 else ds
        return ra, "read" if opcode == 58 else "write", 8, disp
    return None


def scan_words(
    words,
    function_starts,
    *,
    min_offset=0x2D0000,
    max_offset=0x2D7FFF,
    lookahead=12,
):
    out = []
    seen = set()
    for i, (pc, word) in enumerate(words):
        if word >> 26 != 15:  # addis
            continue
        derived = (word >> 21) & 31
        root = (word >> 16) & 31
        if _s16(word & 0xFFFF) != ROOT_HI16:
            continue
        owner_i = bisect.bisect_right(function_starts, pc) - 1
        owner = function_starts[owner_i] if owner_i >= 0 else pc
        for ins_pc, ins in words[i + 1:i + 1 + lookahead]:
            mem = _decode_memory(ins)
            if mem is not None:
                ra, kind, width, disp = mem
                if ra != derived:
                    continue
                offset = (ROOT_HI16 << 16) + disp
            elif ins >> 26 == 14 and ((ins >> 16) & 31) == derived:
                kind, width = "address", 0
                offset = (ROOT_HI16 << 16) + _s16(ins & 0xFFFF)
            else:
                continue
            if not min_offset <= offset <= max_offset:
                continue
            key = (ins_pc, offset, kind)
            if key in seen:
                continue
            seen.add(key)
            out.append({
                "function": owner,
                "addis_pc": pc,
                "instruction": ins_pc,
                "root_register": root,
                "offset": offset,
                "kind": kind,
                "width": width,
            })
    return out


def scan_elf(blob, min_offset=0x2D0000, max_offset=0x2D7FFF):
    segments = load_segments(blob)
    starts = recover_opd_functions(blob, segments)
    words = []
    for seg in segments:
        if not (seg["flags"] & PF_X):
            continue
        end = min(len(blob), seg["file_offset"] + seg["filesz"])
        size = end - seg["file_offset"]
        size -= size % 4
        for rel in range(0, size, 4):
            words.append((
                seg["vaddr"] + rel,
                _u32(blob, seg["file_offset"] + rel),
            ))
    words.sort()
    return scan_words(
        words, starts, min_offset=min_offset, max_offset=max_offset
    )


def summarize(accesses):
    grouped = defaultdict(list)
    for access in accesses:
        grouped[access["offset"]].append(access)
    rows = []
    for offset, group in grouped.items():
        kinds = Counter(a["kind"] for a in group)
        functions = sorted({a["function"] for a in group})
        rows.append({
            "offset": f"0x{offset:08X}",
            "access_count": len(group),
            "function_count": len(functions),
            "read_count": kinds["read"],
            "write_count": kinds["write"],
            "address_count": kinds["address"],
            "width_bytes": sorted({a["width"] for a in group if a["width"]}),
            "functions": [f"0x{x:08X}" for x in functions],
            "sites": [{
                "function": f"0x{a['function']:08X}",
                "instruction": f"0x{a['instruction']:08X}",
                "kind": a["kind"],
                "width": a["width"],
            } for a in sorted(group, key=lambda x: x["instruction"])],
        })
    return sorted(
        rows, key=lambda r: (-r["access_count"], int(r["offset"], 16))
    )


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("elf", type=Path)
    ap.add_argument("-o", "--output", type=Path)
    ap.add_argument("--min-offset", type=lambda x: int(x, 0), default=0x2D0000)
    ap.add_argument("--max-offset", type=lambda x: int(x, 0), default=0x2D7FFF)
    ap.add_argument("--min-hits", type=int, default=1)
    args = ap.parse_args()

    rows = [
        r for r in summarize(
            scan_elf(args.elf.read_bytes(), args.min_offset, args.max_offset)
        )
        if r["access_count"] >= args.min_hits
    ]
    payload = json.dumps(rows, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(payload, encoding="utf-8")
    else:
        print(payload, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
