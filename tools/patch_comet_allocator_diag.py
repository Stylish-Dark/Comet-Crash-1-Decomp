from __future__ import annotations
import argparse
from pathlib import Path

FUNC_ANCHOR = """void func_001A4C6C(ppu_context* ctx) {
        uint64_t _cs_20 = ctx->gpr[20];"""
FUNC_REPL = """void func_001A4C6C(ppu_context* ctx) {
        const uint64_t comet_diag_mspace = ctx->gpr[3];
        const uint64_t comet_diag_mem = ctx->gpr[4];
        const uint64_t comet_diag_caller_lr = ctx->lr;
        uint64_t _cs_20 = ctx->gpr[20];"""
FAIL_ANCHOR = """loc_001A4E8C:
        ctx->lr = 0x001A4E90; func_0019427C(ctx); DRAIN_TRAMPOLINE(ctx);
        /* nop */;"""
MARKER = "[COMET-ALLOC-CORRUPTION]"
DIAG = r'''loc_001A4E8C:
        {
            const uint32_t ms=(uint32_t)comet_diag_mspace;
            const uint32_t mem=(uint32_t)comet_diag_mem;
            const uint32_t chunk=mem ? (mem-8u) : 0u;
            const uint32_t head=chunk ? vm_read32((uint64_t)chunk+4u) : 0u;
            const uint32_t csize=head & ~3u;
            const uint32_t next=(chunk && csize < 0x40000000u) ? (chunk+csize) : 0u;
            const uint32_t next_head=next ? vm_read32((uint64_t)next+4u) : 0u;
            fprintf(stderr, "[COMET-ALLOC-CORRUPTION] caller_lr=0x%08X mspace=0x%08X mem=0x%08X chunk=0x%08X head=0x%08X size=0x%08X pinuse=%u cinuse=%u next=0x%08X next_head=0x%08X\n",
                    (unsigned)comet_diag_caller_lr, ms, mem, chunk, head, csize, head&1u, (head>>1)&1u, next, next_head);
            if (ms) {
                fprintf(stderr, "[COMET-ALLOC-STATE] smallmap=0x%08X treemap=0x%08X dvsize=0x%08X topsize=0x%08X least=0x%08X dv=0x%08X top=0x%08X\n",
                        vm_read32((uint64_t)ms+0x00), vm_read32((uint64_t)ms+0x04),
                        vm_read32((uint64_t)ms+0x08), vm_read32((uint64_t)ms+0x0C),
                        vm_read32((uint64_t)ms+0x10), vm_read32((uint64_t)ms+0x14),
                        vm_read32((uint64_t)ms+0x18));
            }
        }
        ctx->lr = 0x001A4E90; func_0019427C(ctx); DRAIN_TRAMPOLINE(ctx);
        /* nop */;'''

def patch_text(text: str) -> tuple[str, bool]:
    if MARKER in text:
        return text, False
    if text.count(FUNC_ANCHOR) != 1:
        raise ValueError(f"expected one Comet mspace_free function anchor, found {text.count(FUNC_ANCHOR)}")
    if text.count(FAIL_ANCHOR) != 1:
        raise ValueError(f"expected one Comet allocator abort site, found {text.count(FAIL_ANCHOR)}")
    text=text.replace(FUNC_ANCHOR,FUNC_REPL,1)
    text=text.replace(FAIL_ANCHOR,DIAG,1)
    return text, True

def patch_file(path: Path) -> bool:
    src=path.read_bytes().decode("latin-1")
    out,changed=patch_text(src)
    if changed:
        path.write_bytes(out.encode("latin-1"))
    return changed

def main() -> int:
    ap=argparse.ArgumentParser(description="Instrument Comet Crash mspace_free corruption before the title aborts")
    ap.add_argument("source",type=Path,help="generated ppu_recomp_*.cpp containing func_001A4C6C")
    a=ap.parse_args()
    changed=patch_file(a.source)
    print(f"Comet allocator diagnostic patch: {'applied' if changed else 'already present'}")
    return 0

if __name__=="__main__":
    raise SystemExit(main())
