#!/usr/bin/env python3
"""Recover source-file/string anchors from a stripped PS3 PPU ELF.

Comet Crash was shipped stripped, but the executable still contains a small
number of source-file/assertion strings. PPU code normally reaches those
strings through the module TOC. This tool finds TOC-relative loads whose TOC
slot contains a pointer to printable ASCII and assigns each reference to the
owning function from ppu_loader's function inventory.

The output is metadata only; it never copies executable bytes into the repo.
"""
from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path

from capstone import CS_ARCH_PPC, CS_MODE_64, CS_MODE_BIG_ENDIAN, CS_OP_MEM, Cs
from capstone.ppc import PPC_REG_R2


def _u16(b: bytes, o: int) -> int:
    return struct.unpack_from(">H", b, o)[0]


def _u32(b: bytes, o: int) -> int:
    return struct.unpack_from(">I", b, o)[0]


def _u64(b: bytes, o: int) -> int:
    return struct.unpack_from(">Q", b, o)[0]


def load_segments(blob: bytes) -> list[tuple[int, int, int]]:
    if blob[:4] != b"\x7fELF" or blob[4] != 2 or blob[5] != 2:
        raise ValueError("expected ELF64 big-endian input")
    phoff = _u64(blob, 0x20)
    phentsize = _u16(blob, 0x36)
    phnum = _u16(blob, 0x38)
    out: list[tuple[int, int, int]] = []
    for i in range(phnum):
        o = phoff + i * phentsize
        if _u32(blob, o) != 1:  # PT_LOAD
            continue
        p_offset = _u64(blob, o + 0x08)
        p_vaddr = _u64(blob, o + 0x10)
        p_filesz = _u64(blob, o + 0x20)
        out.append((p_vaddr, p_offset, p_filesz))
    return out


def va_to_offset(segments: list[tuple[int, int, int]], va: int) -> int | None:
    for vaddr, offset, filesz in segments:
        if vaddr <= va < vaddr + filesz:
            return offset + (va - vaddr)
    return None


def read_c_string(blob: bytes, segments: list[tuple[int, int, int]], va: int, max_len: int = 300) -> str | None:
    off = va_to_offset(segments, va)
    if off is None or off >= len(blob):
        return None
    end = blob.find(b"\0", off, min(len(blob), off + max_len))
    if end < 0 or end - off < 3:
        return None
    raw = blob[off:end]
    try:
        s = raw.decode("ascii")
    except UnicodeDecodeError:
        return None
    if any(ord(ch) < 0x20 and ch not in "\t\r\n" for ch in s):
        return None
    return s


def load_functions(path: Path) -> list[dict]:
    rows = json.loads(path.read_text(encoding="utf-8"))
    out = []
    for row in rows:
        start = int(row["start"], 0) if isinstance(row["start"], str) else int(row["start"])
        end = int(row["end"], 0) if isinstance(row["end"], str) else int(row["end"])
        toc = int(row["toc"], 0) if isinstance(row["toc"], str) else int(row["toc"])
        out.append({"start": start, "end": end, "toc": toc})
    out.sort(key=lambda x: x["start"])
    return out


def recover_refs(elf: Path, functions_json: Path) -> list[dict]:
    blob = elf.read_bytes()
    segments = load_segments(blob)
    functions = load_functions(functions_json)

    md = Cs(CS_ARCH_PPC, CS_MODE_64 | CS_MODE_BIG_ENDIAN)
    md.detail = True
    refs: list[dict] = []
    seen: set[tuple[int, int]] = set()

    # Disassemble each loader-discovered function independently. The executable
    # PT_LOAD also contains embedded data, so one monolithic Capstone pass can
    # terminate early when it crosses a data island.
    for owner in functions:
        start, end = owner["start"], owner["end"]
        start_off = va_to_offset(segments, start)
        end_off = va_to_offset(segments, end - 1)
        if start_off is None or end_off is None or end <= start:
            continue
        code = blob[start_off : end_off + 1]
        for ins in md.disasm(code, start):
            for op in ins.operands:
                if op.type != CS_OP_MEM or op.mem.base != PPC_REG_R2:
                    continue
                slot_va = (owner["toc"] + op.mem.disp) & 0xFFFFFFFFFFFFFFFF
                slot_off = va_to_offset(segments, slot_va)
                if slot_off is None or slot_off + 4 > len(blob):
                    continue
                target = _u32(blob, slot_off)
                text = read_c_string(blob, segments, target)
                if text is None:
                    continue
                key = (ins.address, target)
                if key in seen:
                    continue
                seen.add(key)
                refs.append({
                    "function": f"0x{owner['start']:08X}",
                    "instruction": f"0x{ins.address:08X}",
                    "toc_slot": f"0x{slot_va:08X}",
                    "target": f"0x{target:08X}",
                    "text": text,
                })
    return refs


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("elf", type=Path)
    ap.add_argument("functions", type=Path, help="EBOOT.functions.json from ppu_loader.py")
    ap.add_argument("-o", "--output", type=Path)
    ap.add_argument("--contains", help="only show refs whose text contains this substring")
    args = ap.parse_args()

    refs = recover_refs(args.elf, args.functions)
    if args.contains:
        needle = args.contains.lower()
        refs = [r for r in refs if needle in r["text"].lower()]

    payload = json.dumps(refs, indent=2, ensure_ascii=False) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(payload, encoding="utf-8")
    else:
        print(payload, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
