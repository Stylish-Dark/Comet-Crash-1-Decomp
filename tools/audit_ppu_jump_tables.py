from __future__ import annotations
import argparse, json, re
from pathlib import Path
from ps3_probe import PT_LOAD, parse_elf64_header, phdrs, read_vaddr, u32

BCTR = 0x4E800420
SWITCH_RE = re.compile(r'switch \(\(uint32_t\)ctx->ctr\) \{(.*?)default:', re.S)
CASE_RE = re.compile(r'case 0x([0-9A-Fa-f]{8})u:\s*goto loc_[0-9A-Fa-f]{8};')


def _xo(insn: int) -> int:
    return (insn >> 1) & 0x3FF


def _rt(insn: int) -> int:
    return (insn >> 21) & 31


def _ra(insn: int) -> int:
    return (insn >> 16) & 31


def _rb(insn: int) -> int:
    return (insn >> 11) & 31


def _is_mtspr_ctr(insn: int) -> bool:
    return (insn >> 26) == 31 and _xo(insn) == 467 and _ra(insn) == 9 and _rb(insn) == 0


def _is_add(insn: int) -> bool:
    return (insn >> 26) == 31 and _xo(insn) == 266


def _is_extsw(insn: int) -> bool:
    return (insn >> 26) == 31 and _xo(insn) == 986


def _is_lwzx(insn: int) -> bool:
    return (insn >> 26) == 31 and _xo(insn) == 23


def _exec_segments(data: bytes) -> list[dict]:
    h = parse_elf64_header(data)
    return [p for p in phdrs(data, h) if p['type'] == PT_LOAD and (p.get('flags', 0) & 1) and p['filesz']]


def _is_exec_addr(addr: int, exec_segments: list[dict]) -> bool:
    return any(int(p['vaddr']) <= addr < int(p['vaddr']) + int(p['filesz']) for p in exec_segments)


def _read_word(data: bytes, ph: list[dict], addr: int) -> int | None:
    raw = read_vaddr(data, ph, addr, 4)
    return u32(raw, 0) if raw is not None and len(raw) == 4 else None


def _structural_inline_relative(data: bytes, ph: list[dict], bctr: int) -> bool:
    mtctr = _read_word(data, ph, bctr - 4)
    if mtctr is None or not _is_mtspr_ctr(mtctr):
        return False
    ctr_reg = _rt(mtctr)
    prev = []
    for back in range(2, 13):
        w = _read_word(data, ph, bctr - back * 4)
        if w is not None:
            prev.append(w)
    for idx, insn in enumerate(prev[:6]):
        if not _is_add(insn) or _rt(insn) != ctr_reg:
            continue
        sources = {_ra(insn), _rb(insn)}
        earlier = prev[idx + 1:10]
        extsw_dests = {_ra(w) for w in earlier if _is_extsw(w)}
        lwzx_dests = {_rt(w) for w in earlier if _is_lwzx(w)}
        if sources & extsw_dests & lwzx_dests:
            return True
    return False


def _decode_inline_targets(data: bytes, ph: list[dict], exec_segments: list[dict], bctr: int) -> list[int]:
    base = (bctr + 4) & 0xFFFFFFFF
    targets: list[int] = []
    for k in range(256):
        ea = (base + k * 4) & 0xFFFFFFFF
        forward = [t for t in targets if t > base]
        if forward and ea >= min(forward):
            break
        value = _read_word(data, ph, ea)
        if value is None:
            break
        offset = value - (1 << 32) if value & 0x80000000 else value
        target = (base + offset) & 0xFFFFFFFF
        if _is_exec_addr(target, exec_segments) and target % 4 == 0:
            targets.append(target)
        elif targets or k >= 4:
            break
    return targets


def _generated_switch_sets(generated_dir: Path) -> list[frozenset[int]]:
    switches: list[frozenset[int]] = []
    for path in sorted(generated_dir.glob('ppu_recomp_*.cpp')):
        text = path.read_text(encoding='latin-1')
        for body in SWITCH_RE.findall(text):
            cases = frozenset(int(x, 16) for x in CASE_RE.findall(body))
            if cases:
                switches.append(cases)
    return switches


def audit(elf_path: Path, generated_dir: Path) -> dict[str, object]:
    data = elf_path.read_bytes()
    h = parse_elf64_header(data)
    ph = phdrs(data, h)
    exec_segments = _exec_segments(data)
    generated_switches = _generated_switch_sets(generated_dir)
    candidates = []
    for seg in exec_segments:
        lo = int(seg['vaddr'])
        hi = lo + int(seg['filesz'])
        for addr in range(lo, hi - 3, 4):
            if _read_word(data, ph, addr) != BCTR:
                continue
            if not _structural_inline_relative(data, ph, addr):
                continue
            targets = _decode_inline_targets(data, ph, exec_segments, addr)
            if len(targets) < 2:
                continue
            target_set = frozenset(targets)
            recovered = target_set in generated_switches
            candidates.append({
                'bctr': addr,
                'table_base': (addr + 4) & 0xFFFFFFFF,
                'entries': len(targets),
                'unique_targets': len(target_set),
                'targets': sorted(target_set),
                'recovered': recovered,
            })
    missing = [x for x in candidates if not x['recovered']]
    return {
        'structural_inline_tables': len(candidates),
        'generated_ctr_switches': len(generated_switches),
        'recovered_inline_tables': len(candidates) - len(missing),
        'missing_inline_tables': len(missing),
        'missing': missing,
        'candidates': candidates,
    }


def main() -> int:
    ap = argparse.ArgumentParser(description='Audit structural inline signed-relative PPU jump tables against generated ctr switches')
    ap.add_argument('elf', type=Path)
    ap.add_argument('generated', type=Path)
    a = ap.parse_args()
    report = audit(a.elf, a.generated)
    print(json.dumps(report, indent=2))
    return 1 if report['missing_inline_tables'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
