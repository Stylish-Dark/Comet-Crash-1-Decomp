from __future__ import annotations
import struct
from pathlib import Path

def parse_sfo_bytes(data: bytes) -> dict[str, object]:
    if len(data) < 0x14 or data[:4] != b"\0PSF":
        raise ValueError("not a PARAM.SFO")
    key_base, data_base, count = struct.unpack_from("<III", data, 8)
    out: dict[str, object] = {}
    for i in range(count):
        off = 0x14 + i * 0x10
        if off + 0x10 > len(data):
            raise ValueError("truncated PARAM.SFO index")
        key_off, fmt, size, _max_size, value_off = struct.unpack_from("<HHIII", data, off)
        ks = key_base + key_off
        ke = data.find(b"\0", ks)
        if ks >= len(data) or ke < 0:
            continue
        key = data[ks:ke].decode("utf-8", "replace")
        vs = data_base + value_off
        raw = data[vs:vs + size]
        if fmt in (0x0204, 0x0004):
            value: object = raw.rstrip(b"\0").decode("utf-8", "replace")
        elif fmt == 0x0404 and len(raw) >= 4:
            value = struct.unpack_from("<I", raw)[0]
        else:
            value = raw
        out[key] = value
    return out

def parse_sfo(path: str | Path) -> dict[str, object]:
    return parse_sfo_bytes(Path(path).read_bytes())
