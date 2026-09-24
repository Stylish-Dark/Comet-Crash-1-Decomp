from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path

REFERENCE_SHA256 = "3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6"
PATCHED_SHA256 = "f0f4ec0b1c1d8a673335964cb64db556f33485019631bf685e56407f0f022bb9"
PATCH_VADDR = 0x001A4E8C
EXPECTED = bytes.fromhex("4bfef3f1")  # bl 0x0019427c; return address 0x001A4E90
REPLACEMENT = bytes.fromhex("60000000")  # nop

def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def file_offset_for_vaddr(data: bytes, vaddr: int) -> int:
    if data[:4] != b"\x7fELF" or data[4] != 2 or data[5] != 2:
        raise ValueError("expected 64-bit big-endian PS3 ELF")
    phoff=struct.unpack_from(">Q",data,32)[0]
    phentsize=struct.unpack_from(">H",data,54)[0]
    phnum=struct.unpack_from(">H",data,56)[0]
    for i in range(phnum):
        off=phoff+i*phentsize
        p_type,p_flags,p_offset,p_vaddr,p_paddr,p_filesz,p_memsz,p_align=struct.unpack_from(">IIQQQQQQ",data,off)
        if p_type == 1 and p_vaddr <= vaddr < p_vaddr + p_filesz:
            return p_offset + (vaddr-p_vaddr)
    raise ValueError(f"guest address 0x{vaddr:08X} is not file-backed")

def patch_bytes(data: bytes, *, require_reference_hash: bool=True) -> bytes:
    if require_reference_hash:
        actual=sha256_bytes(data)
        if actual != REFERENCE_SHA256:
            raise ValueError(f"allocator diagnostic patch requires reference ELF {REFERENCE_SHA256}, got {actual}")
    out=bytearray(data)
    off=file_offset_for_vaddr(out,PATCH_VADDR)
    found=bytes(out[off:off+4])
    if found != EXPECTED:
        raise ValueError(f"unexpected instruction at 0x{PATCH_VADDR:08X}: {found.hex()} != {EXPECTED.hex()}")
    out[off:off+4]=REPLACEMENT
    patched=bytes(out)
    if require_reference_hash and sha256_bytes(patched) != PATCHED_SHA256:
        raise RuntimeError("patched ELF fingerprint drifted")
    return patched

def patch_file(source: Path, output: Path) -> None:
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_bytes(patch_bytes(source.read_bytes()))
    print(f"patched guest allocator abort call 0x{PATCH_VADDR:08X}: {source} -> {output}")
    print(f"SHA-256: {sha256_bytes(output.read_bytes())}")

def main() -> int:
    ap=argparse.ArgumentParser(description="Create the Boot Fix 3 diagnostic ELF by bypassing one observed allocator abort call")
    ap.add_argument("source",type=Path)
    ap.add_argument("output",type=Path)
    a=ap.parse_args()
    patch_file(a.source,a.output)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
