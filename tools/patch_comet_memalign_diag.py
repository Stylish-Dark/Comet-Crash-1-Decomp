from __future__ import annotations
import argparse
from pathlib import Path

WRAP_ANCHOR = """void func_001A80D0(ppu_context* ctx) {
        vm_write64(ctx->gpr[1] + -0x70, ctx->gpr[1]); ctx->gpr[1] += -0x70;"""
WRAP_REPL = """void func_001A80D0(ppu_context* ctx) {
        const uint32_t comet_ma_caller_lr=(uint32_t)ctx->lr;
        const uint32_t comet_ma_align=(uint32_t)ctx->gpr[3];
        const uint32_t comet_ma_bytes=(uint32_t)ctx->gpr[4];
        vm_write64(ctx->gpr[1] + -0x70, ctx->gpr[1]); ctx->gpr[1] += -0x70;"""
WRAP_RETURN_ANCHOR = """        ctx->lr = 0x001A80F0; func_001A78E0(ctx); DRAIN_TRAMPOLINE(ctx);
        /* nop */;
        ctx->gpr[0] = vm_read64(ctx->gpr[1] + 0x80);"""
WRAP_RETURN_REPL = r'''        ctx->lr = 0x001A80F0; func_001A78E0(ctx); DRAIN_TRAMPOLINE(ctx);
        /* nop */;
        {
            const uint32_t result=(uint32_t)ctx->gpr[3];
            const uint32_t ms=vm_read32(ctx->gpr[2] + -0x4678);
            const uint32_t least=ms ? vm_read32((uint64_t)ms+0x10) : 0u;
            if (result && least && result < least) {
                fprintf(stderr,"[COMET-MEMALIGN-WRAPPER-LOW] caller_lr=0x%08X align=0x%08X bytes=0x%08X mspace=0x%08X least=0x%08X result=0x%08X\n",
                        comet_ma_caller_lr,comet_ma_align,comet_ma_bytes,ms,least,result);
            }
        }
        ctx->gpr[0] = vm_read64(ctx->gpr[1] + 0x80);'''

CORE_ANCHOR = """void func_001A75E8(ppu_context* ctx) {
        uint64_t _cs_24 = ctx->gpr[24];"""
CORE_REPL = """void func_001A75E8(ppu_context* ctx) {
        const uint32_t comet_mma_caller_lr=(uint32_t)ctx->lr;
        const uint32_t comet_mma_ms=(uint32_t)ctx->gpr[3];
        const uint32_t comet_mma_align=(uint32_t)ctx->gpr[4];
        const uint32_t comet_mma_bytes=(uint32_t)ctx->gpr[5];
        uint32_t comet_mma_malloc_request=0;
        uint64_t _cs_24 = ctx->gpr[24];"""
MALLOC_ANCHOR = """        ctx->gpr[3] = ctx->gpr[27] | ctx->gpr[27];
        ctx->lr = 0x001A7708; func_001A5A90(ctx); DRAIN_TRAMPOLINE(ctx);
        { int64_t a = (int32_t)ctx->gpr[3]; int64_t b = (int64_t)0;"""
MALLOC_REPL = r'''        ctx->gpr[3] = ctx->gpr[27] | ctx->gpr[27];
        comet_mma_malloc_request=(uint32_t)ctx->gpr[4];
        ctx->lr = 0x001A7708; func_001A5A90(ctx); DRAIN_TRAMPOLINE(ctx);
        {
            const uint32_t result=(uint32_t)ctx->gpr[3];
            const uint32_t least=comet_mma_ms ? vm_read32((uint64_t)comet_mma_ms+0x10) : 0u;
            if (result && least && result < least) {
                fprintf(stderr,"[COMET-MEMALIGN-MALLOC-LOW] caller_lr=0x%08X align=0x%08X bytes=0x%08X malloc_request=0x%08X mspace=0x%08X least=0x%08X result=0x%08X\n",
                        comet_mma_caller_lr,comet_mma_align,comet_mma_bytes,comet_mma_malloc_request,comet_mma_ms,least,result);
            }
        }
        { int64_t a = (int32_t)ctx->gpr[3]; int64_t b = (int64_t)0;'''
FINAL_ANCHOR = """loc_001A76A4:
        ctx->gpr[3] = ppc_rldicl(ctx->gpr[0], 0, 32);
        ctx->gpr[0] = vm_read64(ctx->gpr[1] + 0xC0);"""
FINAL_REPL = r'''loc_001A76A4:
        ctx->gpr[3] = ppc_rldicl(ctx->gpr[0], 0, 32);
        {
            const uint32_t result=(uint32_t)ctx->gpr[3];
            const uint32_t least=comet_mma_ms ? vm_read32((uint64_t)comet_mma_ms+0x10) : 0u;
            if (result && least && result < least) {
                fprintf(stderr,"[COMET-MEMALIGN-CORE-LOW] caller_lr=0x%08X align=0x%08X bytes=0x%08X mspace=0x%08X least=0x%08X result=0x%08X r24=0x%08X r25=0x%08X r26=0x%08X r29=0x%08X\n",
                        comet_mma_caller_lr,comet_mma_align,comet_mma_bytes,comet_mma_ms,least,result,
                        (uint32_t)ctx->gpr[24],(uint32_t)ctx->gpr[25],(uint32_t)ctx->gpr[26],(uint32_t)ctx->gpr[29]);
            }
        }
        ctx->gpr[0] = vm_read64(ctx->gpr[1] + 0xC0);'''

MARKER="[COMET-MEMALIGN-WRAPPER-LOW]"

def patch_text(text: str) -> tuple[str, bool]:
    if MARKER in text:
        return text, False
    pairs=[
        (WRAP_ANCHOR,WRAP_REPL,"wrapper entry"),
        (WRAP_RETURN_ANCHOR,WRAP_RETURN_REPL,"wrapper return"),
        (CORE_ANCHOR,CORE_REPL,"core entry"),
        (MALLOC_ANCHOR,MALLOC_REPL,"backing malloc"),
        (FINAL_ANCHOR,FINAL_REPL,"core final return"),
    ]
    for old,_,name in pairs:
        count=text.count(old)
        if count != 1:
            raise ValueError(f"expected one Comet memalign {name} anchor, found {count}")
    for old,new,_ in pairs:
        text=text.replace(old,new,1)
    return text, True

def patch_file(path: Path) -> bool:
    src=path.read_bytes().decode("latin-1")
    out,changed=patch_text(src)
    if changed:
        path.write_bytes(out.encode("latin-1"))
    return changed

def main() -> int:
    ap=argparse.ArgumentParser(description="Instrument Comet Crash aligned allocator low-pointer returns")
    ap.add_argument("source",type=Path,help="generated ppu_recomp_*.cpp containing the Comet allocator")
    a=ap.parse_args()
    changed=patch_file(a.source)
    print(f"Comet memalign diagnostic patch: {'applied' if changed else 'already present'}")
    return 0

if __name__=="__main__":
    raise SystemExit(main())
