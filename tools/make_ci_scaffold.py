from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

PPU_STUB = r'''#include "ppu_recomp.h"

extern "C" void func_00010000(ppu_context*) {}

extern "C" {
extern const func_entry function_table[] = {
    {0x00010000u, func_00010000, "func_00010000"},
    {0, nullptr, nullptr},
};
extern const uint64_t function_table_count = 1;
}
'''

SPU_STUB = '''/* Synthetic CI-only SPU translation unit.\n * The Windows scaffold build checks compiler/link/runtime integration; it does\n * not execute guest SPU code. Real lifts are supplied only from user-owned data.\n */\n'''

REGISTRY_STUB = '''/* Synthetic CI-only workload registry.\n * Real Comet builds replace this ignored generated file with the fingerprinted\n * registry produced by ps3recomp/tools/build_spu_workloads.py.\n */\n'''


def write_stub_sources(recomp: Path, spu: Path, registry: Path) -> None:
    recomp.mkdir(parents=True, exist_ok=True)
    (spu / "ci_stub").mkdir(parents=True, exist_ok=True)
    registry.parent.mkdir(parents=True, exist_ok=True)
    (recomp / "ppu_recomp_ci.cpp").write_text(PPU_STUB, encoding="utf-8", newline="\n")
    (spu / "ci_stub" / "spu_recomp.c").write_text(SPU_STUB, encoding="utf-8", newline="\n")
    registry.write_text(REGISTRY_STUB, encoding="utf-8", newline="\n")


def stage(ps3recomp: Path, generated: Path, work: Path) -> None:
    ps3recomp = ps3recomp.resolve()
    generated = generated.resolve()
    work = work.resolve()
    make_smoke = ps3recomp / "tools" / "make_smoke_elf.py"
    if not make_smoke.is_file():
        raise FileNotFoundError(f"missing pinned ps3recomp smoke generator: {make_smoke}")

    recomp = generated / "recompiled"
    spu = generated / "spu"
    registry = generated / "spu_workloads.c"
    if generated.exists():
        shutil.rmtree(generated)
    generated.mkdir(parents=True)
    work.mkdir(parents=True, exist_ok=True)

    # The upstream smoke generator writes ppu_recomp.h directly from the same
    # HEADER_PREAMBLE as the lifter.  Using it makes this compile gate sensitive
    # to ABI drift without committing any generated/proprietary title code.
    smoke_elf = work / "ci_smoke.elf"
    header = recomp / "ppu_recomp.h"
    recomp.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [sys.executable, str(make_smoke), "--out", str(smoke_elf), "--header", str(header)],
        check=True,
    )
    if not header.is_file():
        raise RuntimeError(f"smoke generator did not produce {header}")
    write_stub_sources(recomp, spu, registry)
    print(f"CI PPU header: {header}")
    print(f"CI PPU stub:   {recomp / 'ppu_recomp_ci.cpp'}")
    print(f"CI SPU stub:   {spu / 'ci_stub' / 'spu_recomp.c'}")


def main() -> int:
    ap = argparse.ArgumentParser(description="Stage non-proprietary generated-code fixtures for the Windows port compile gate")
    ap.add_argument("--ps3recomp", type=Path, required=True)
    ap.add_argument("--generated", type=Path, default=ROOT / "generated" / "ci")
    ap.add_argument("--work", type=Path, default=ROOT / "work" / "ci")
    a = ap.parse_args()
    stage(a.ps3recomp, a.generated, a.work)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
