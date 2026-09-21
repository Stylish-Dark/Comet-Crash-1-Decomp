"""Rebuild a plain ELF from the user's retail Comet Crash NPEB00142 EBOOT.BIN.

This is intentionally title-specific. Comet Crash's NPDRM header uses the FREE
license class, so no RAP/RIF or user secret is needed. The crypto constants are
the public PS3 NPDRM/platform keys used by interoperability projects such as
RPCS3. The output follows RPCS3's SELF MakeElf behaviour: ELF/PHDR/SHDR data
plus decrypted program segments; non-alloc debug/string-table payloads are not
reconstructed because RPCS3 does not copy them either.
"""
from __future__ import annotations

import argparse, hashlib, struct, zlib
from pathlib import Path

try:
    from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
except ImportError as exc:  # pragma: no cover - exercised by bootstrap instead
    raise SystemExit("cryptography is required: py -3 -m pip install -r requirements.txt") from exc

SCE_MAGIC = 0x53434500
EXPECTED_CONTENT_ID = "UP0001-NPEB00142_00-0000111122223333"
EXPECTED_AUTH_ID = 0x1010000001000003
EXPECTED_PROGRAM_TYPE = 8  # NPDRM application SELF
EXPECTED_KEY_REVISION = 1
NPDRM_LICENSE_FREE = 3

NP_KLIC_FREE = bytes.fromhex("72F990788F9CFF745725F08E4C128387")
NP_KLIC_KEY = bytes.fromhex("F2FBCA7A75B04EDC1390638CCDFDD1EE")
NPDRM_REV1_ERK = bytes.fromhex("F9EDD0301F770FABBA8863D9897F0FEA6551B09431F61312654E28F43533EA6B")
NPDRM_REV1_RIV = bytes.fromhex("A551CCB4A42C37A734A2B4F9657D5540")


def _be(fmt: str, b: bytes | bytearray, o: int):
    return struct.unpack_from(">" + fmt, b, o)


def _aes_decrypt(key: bytes, mode, data: bytes) -> bytes:
    dec = Cipher(algorithms.AES(key), mode).decryptor()
    return dec.update(data) + dec.finalize()


def _aes_ctr(key: bytes, iv: bytes, data: bytes) -> bytes:
    # CTR encryption and decryption are the same operation.
    enc = Cipher(algorithms.AES(key), modes.CTR(iv)).encryptor()
    return enc.update(data) + enc.finalize()


def decrypt_metadata_info(encrypted: bytes) -> bytes:
    """Remove Comet Crash's FREE-NPDRM layer and revision-1 SELF layer."""
    if len(encrypted) != 0x40:
        raise ValueError("metadata info must be exactly 64 bytes")
    npdrm_key = _aes_decrypt(NP_KLIC_KEY, modes.ECB(), NP_KLIC_FREE)
    plain = _aes_decrypt(npdrm_key, modes.CBC(bytes(16)), encrypted)
    return _aes_decrypt(NPDRM_REV1_ERK, modes.CBC(NPDRM_REV1_RIV), plain)


def _find_npdrm(data: bytes, supp_off: int, supp_size: int) -> tuple[int, str]:
    pos, end = supp_off, supp_off + supp_size
    while pos + 16 <= end and pos + 16 <= len(data):
        typ, size, _next = _be("IIQ", data, pos)
        if size < 16 or pos + size > len(data):
            break
        if typ == 3 and size >= 0x80:
            q = pos + 16
            magic, _version, license_type, _app_type = _be("IIII", data, q)
            if magic != 0x4E504400:  # NPD\0
                raise ValueError("NPDRM control header has invalid magic")
            cid = data[q + 16:q + 64].split(b"\0", 1)[0].decode("ascii", "strict")
            return license_type, cid
        pos += size
    raise ValueError("NPDRM control header not found")


def decrypt_self(input_path: Path, output_path: Path) -> dict:
    data = input_path.read_bytes()
    if len(data) < 0x100:
        raise ValueError("SELF is too small")

    magic, _hver, flags, sce_type, se_meta, se_hsize, elf_size = _be("IIHHIQQ", data, 0)
    if magic != SCE_MAGIC or sce_type != 1:
        raise ValueError("expected a PS3 SELF")
    if flags != EXPECTED_KEY_REVISION:
        raise ValueError(f"expected SELF key revision {EXPECTED_KEY_REVISION}, got 0x{flags:04X}")
    if se_hsize > len(data):
        raise ValueError("SELF header extends beyond file")

    ext = _be("10Q", data, 0x20)
    _ext_ver, app_off, elf_off, phdr_off, shdr_off, _seginfo, _veroff, supp_off, supp_size, _pad = ext
    auth_id, _vendor, program_type, _sce_version, _app_pad = _be("QIIQQ", data, app_off)
    if auth_id != EXPECTED_AUTH_ID or program_type != EXPECTED_PROGRAM_TYPE:
        raise ValueError("SELF is not the expected Comet Crash NPDRM application")

    license_type, content_id = _find_npdrm(data, supp_off, supp_size)
    if license_type != NPDRM_LICENSE_FREE:
        raise ValueError(f"expected FREE NPDRM license type 3, got {license_type}")
    if content_id != EXPECTED_CONTENT_ID:
        raise ValueError(f"wrong NPDRM content id: {content_id}")

    # SELF carries a copy of the original ELF header and header tables.
    elf_header = data[elf_off:elf_off + 64]
    if len(elf_header) != 64 or elf_header[:6] != b"\x7fELF\x02\x02":
        raise ValueError("SELF does not contain the expected ELF64 big-endian header")
    (e_type, e_machine, _e_version, _e_entry, e_phoff, e_shoff, _e_flags,
     e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, _e_shstrndx) = _be("HHIQQQIHHHHHH", elf_header, 16)
    if e_machine != 21 or e_ehsize != 64:
        raise ValueError("embedded ELF is not a PS3 PPC64 executable")

    phdrs = []
    for i in range(e_phnum):
        off = phdr_off + i * e_phentsize
        if off + e_phentsize > len(data):
            raise ValueError("truncated SELF program-header copy")
        phdrs.append(_be("IIQQQQQQ", data, off))

    # Metadata info has an outer FREE-NPDRM layer, then the revision-1 SELF layer.
    mi_off = se_meta + 0x20
    metadata_info = decrypt_metadata_info(data[mi_off:mi_off + 0x40])
    metadata_key = metadata_info[:16]
    key_pad = metadata_info[16:32]
    metadata_iv = metadata_info[32:48]
    iv_pad = metadata_info[48:64]
    if key_pad != bytes(16) or iv_pad != bytes(16):
        raise ValueError("SELF metadata key padding is invalid; decryption failed")

    mh_off = mi_off + 0x40
    metadata = _aes_ctr(metadata_key, metadata_iv, data[mh_off:se_hsize])
    if len(metadata) < 32:
        raise ValueError("decrypted metadata header is truncated")
    _siglen, _unk1, section_count, key_count, _opt, _unk2, _unk3 = _be("QIIIIII", metadata, 0)
    if not (1 <= section_count <= 64 and 1 <= key_count <= 256):
        raise ValueError("implausible SELF metadata counts")

    sections = []
    section_table_end = 32 + section_count * 48
    keys_end = section_table_end + key_count * 16
    if keys_end > len(metadata):
        raise ValueError("SELF metadata section/key table is truncated")
    for i in range(section_count):
        sections.append(_be("QQIIIIIIII", metadata, 32 + i * 48))
    data_keys = metadata[section_table_end:keys_end]

    # Match RPCS3 SELFDecrypter::DecryptData/WriteElf: only encrypted sections
    # with valid data-key indices participate in data_buf; type==2 sections are
    # then placed at their original ELF program-header offsets.
    decrypted_by_index: dict[int, bytes] = {}
    for i, sec in enumerate(sections):
        data_off, data_size, _stype, _prog, _hashed, _shaidx, encrypted, key_idx, iv_idx, _compressed = sec
        if encrypted != 3 or key_idx >= key_count or iv_idx >= key_count:
            continue
        if data_off + data_size > len(data):
            raise ValueError(f"SELF metadata section {i} is outside the file")
        key = data_keys[key_idx * 16:(key_idx + 1) * 16]
        iv = data_keys[iv_idx * 16:(iv_idx + 1) * 16]
        decrypted_by_index[i] = _aes_ctr(key, iv, data[data_off:data_off + data_size])

    out = bytearray(elf_size)
    out[:64] = elf_header
    ph_bytes = e_phnum * e_phentsize
    out[e_phoff:e_phoff + ph_bytes] = data[phdr_off:phdr_off + ph_bytes]

    for i, sec in enumerate(sections):
        _data_off, data_size, stype, prog, _hashed, _shaidx, _encrypted, _key_idx, _iv_idx, compressed = sec
        if stype != 2:
            continue
        blob = decrypted_by_index.get(i)
        if blob is None:
            continue
        if prog >= len(phdrs):
            raise ValueError(f"metadata section {i} references missing program header {prog}")
        ph = phdrs[prog]
        p_offset, p_filesz = ph[2], ph[5]
        if compressed == 2:
            blob = zlib.decompress(blob)
        if len(blob) > p_filesz:
            raise ValueError(f"metadata section {i} expands beyond program header")
        if p_offset + len(blob) > len(out):
            raise ValueError(f"program header {prog} lies outside advertised ELF size")
        out[p_offset:p_offset + len(blob)] = blob

    if shdr_off and e_shoff and e_shnum:
        sh_bytes = e_shnum * e_shentsize
        if shdr_off + sh_bytes > len(data) or e_shoff + sh_bytes > len(out):
            raise ValueError("section-header table is truncated")
        out[e_shoff:e_shoff + sh_bytes] = data[shdr_off:shdr_off + sh_bytes]

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(out)
    return {
        "content_id": content_id,
        "license_type": license_type,
        "elf_size": len(out),
        "elf_sha256": hashlib.sha256(out).hexdigest(),
        "metadata_sections": section_count,
        "metadata_keys": key_count,
    }


def main() -> int:
    ap = argparse.ArgumentParser(description="Decrypt the user's Comet Crash NPEB00142 retail EBOOT SELF")
    ap.add_argument("input", type=Path, help="USRDIR/EBOOT.BIN from your own NPEB00142 copy")
    ap.add_argument("-o", "--output", type=Path, required=True, help="output decrypted ELF")
    args = ap.parse_args()
    info = decrypt_self(args.input, args.output)
    for key, value in info.items():
        print(f"{key}: {value}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
