from __future__ import annotations

import argparse
from pathlib import Path

MARKER = "[ppu] FATAL: unresolved guest-text target"
ANCHOR = """        }
    }
}

/* Called when lifted code branches to a function that wasn't in this lift
"""

REPLACEMENT = """        }
    }

    /* COMET_STRICT_UNRESOLVED:
     * Reaching this point means ps3_indirect_call could not resolve the target
     * after normal function lookup / OPD repair / garbage-vcall handling.
     * Returning from an aligned address inside the title's executable range is
     * not a harmless no-op: a lifted bctr may have been used as an in-function
     * switch. The old 0x00052DF4 miss returned out of func_0005207C before its
     * epilogue, leaving guest r1 0x210 bytes low and poisoning later stack
     * accesses. Fail immediately instead of letting corrupted guest state run.
     */
    if ((addr & 3u) == 0u && addr >= 0x00010000u && addr < 0x10000000u) {
        fprintf(stderr,
                "[ppu] FATAL: unresolved guest-text target 0x%08X "
                "(tid=%llu lr=0x%08X sp=0x%08X) -- refusing corrupt return\\n",
                addr, (unsigned long long)ctx->thread_id,
                (uint32_t)ctx->lr, (uint32_t)ctx->gpr[1]);
        ppu_dump_guest_stack(ctx, "unresolved-text");
        fflush(stderr);
        exit(3);
    }
}

/* Called when lifted code branches to a function that wasn't in this lift
"""

def patch_text(text: str) -> tuple[str, bool]:
    if MARKER in text:
        return text, False
    count = text.count(ANCHOR)
    if count != 1:
        raise ValueError(
            f"expected one pinned ps3recomp indirect-dispatch tail anchor, found {count}; upstream changed"
        )
    return text.replace(ANCHOR, REPLACEMENT, 1), True

def patch_file(ps3recomp: Path) -> bool:
    path = ps3recomp / "runtime" / "ppu" / "ppu_loader.cpp"
    if not path.is_file():
        raise FileNotFoundError(path)
    src = path.read_text(encoding="utf-8")
    out, changed = patch_text(src)
    if changed:
        path.write_text(out, encoding="utf-8", newline="\n")
    return changed

def main() -> int:
    ap = argparse.ArgumentParser(
        description="Make unresolved aligned Comet guest-text indirect targets fail fast"
    )
    ap.add_argument("ps3recomp", type=Path)
    a = ap.parse_args()
    changed = patch_file(a.ps3recomp)
    print(f"Comet unresolved guest-text guard: {'applied' if changed else 'already present'}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
