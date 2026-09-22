from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path

from audit_ppu_lift import audit_path as audit_ppu_path
from patch_ppu_lift import patch_file as patch_ppu_file

ROOT = Path(__file__).resolve().parents[1]

PPU_BASE = 0x00010000
PPU_RAW = (0x4E800020).to_bytes(4, "big")  # blr
PPU_FUNCTIONS = [{"start": hex(PPU_BASE), "end": hex(PPU_BASE + len(PPU_RAW))}]

SPU_STUB = """/* Synthetic CI-only SPU translation unit.
 * The Windows scaffold build checks compiler/link/runtime integration; it does
 * not execute guest SPU code. Real lifts are supplied only from user-owned data.
 */
"""

REGISTRY_STUB = """/* Synthetic CI-only workload registry.
 * Real Comet builds replace this ignored generated file with the fingerprinted
 * registry produced by ps3recomp/tools/build_spu_workloads.py.
 */
"""


def write_ppu_input(work: Path) -> tuple[Path, Path]:
    work.mkdir(parents=True, exist_ok=True)
    raw = work / "ci_ppu.bin"
    functions = work / "ci_ppu.functions.json"
    raw.write_bytes(PPU_RAW)
    functions.write_text(json.dumps(PPU_FUNCTIONS, indent=2) + "\n", encoding="utf-8", newline="\n")
    return raw, functions


def write_spu_stub_sources(spu: Path, registry: Path) -> None:
    (spu / "ci_stub").mkdir(parents=True, exist_ok=True)
    registry.parent.mkdir(parents=True, exist_ok=True)
    (spu / "ci_stub" / "spu_recomp.c").write_text(SPU_STUB, encoding="utf-8", newline="\n")
    registry.write_text(REGISTRY_STUB, encoding="utf-8", newline="\n")


def stage(ps3recomp: Path, generated: Path, work: Path) -> None:
    ps3recomp = ps3recomp.resolve()
    generated = generated.resolve()
    work = work.resolve()
    lifter = ps3recomp / "tools" / "ppu_lifter.py"
    if not lifter.is_file():
        raise FileNotFoundError(f"missing pinned ps3recomp PPU lifter: {lifter}")

    recomp = generated / "recompiled"
    spu = generated / "spu"
    registry = generated / "spu_workloads.c"
    if generated.exists():
        shutil.rmtree(generated)
    recomp.mkdir(parents=True, exist_ok=True)

    raw, functions = write_ppu_input(work)
    subprocess.run(
        [
            sys.executable,
            str(lifter),
            str(raw),
            "--raw",
            "--base",
            hex(PPU_BASE),
            "--functions",
            str(functions),
            "--jobs",
            "1",
            "-o",
            str(recomp),
        ],
        check=True,
    )

    header = recomp / "ppu_recomp.h"
    chunks = sorted(recomp.glob("ppu_recomp_*.cpp"))
    if not header.is_file():
        raise RuntimeError(f"pinned lifter did not produce {header}")
    if not chunks:
        raise RuntimeError(f"pinned lifter produced no PPU source chunks under {recomp}")

    patch_totals = {'vsrab': 0, 'vsrb': 0}
    for source in chunks:
        stats = patch_ppu_file(source)
        for key, value in stats.items():
            patch_totals[key] += value
    print(f"CI PPU compatibility patch: {patch_totals}")

    report = audit_ppu_path(recomp)
    if report["unsupported_total"]:
        sample = ", ".join(str(x["instruction"]) for x in report["unsupported"][:5])
        raise RuntimeError(
            f"synthetic pinned PPU lift contains {report['unsupported_total']} unsupported TODO(s): {sample}"
        )

    write_spu_stub_sources(spu, registry)
    print(f"CI PPU input:  {raw} ({len(PPU_RAW)} bytes, one blr)")
    print(f"CI PPU header: {header}")
    print(f"CI PPU chunks: {len(chunks)}")
    print(f"CI SPU stub:   {spu / 'ci_stub' / 'spu_recomp.c'}")


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Stage non-proprietary real-lifter fixtures for the Windows port compile gate"
    )
    ap.add_argument("--ps3recomp", type=Path, required=True)
    ap.add_argument("--generated", type=Path, default=ROOT / "generated" / "ci")
    ap.add_argument("--work", type=Path, default=ROOT / "work" / "ci")
    a = ap.parse_args()
    stage(a.ps3recomp, a.generated, a.work)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
