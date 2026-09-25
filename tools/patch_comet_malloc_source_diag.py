from __future__ import annotations
import argparse
from pathlib import Path

FUNC_ANCHOR = """void func_001A5A90(ppu_context* ctx) {
        uint64_t _cs_19 = ctx->gpr[19];"""
FUNC_REPL = """void func_001A5A90(ppu_context* ctx) {
        const uint32_t comet_malloc_caller_lr=(uint32_t)ctx->lr;
        const uint32_t comet_malloc_ms=(uint32_t)ctx->gpr[3];
        const uint32_t comet_malloc_request=(uint32_t)ctx->gpr[4];
        const char* comet_malloc_r31_source="entry";
        uint64_t _cs_19 = ctx->gpr[19];"""
RETURN_ANCHOR = """loc_001A5B5C:
        ctx->gpr[0] = vm_read64(ctx->gpr[1] + 0x100);
        ctx->gpr[3] = ppc_rldicl(ctx->gpr[31], 0, 32);"""
RETURN_REPL = r'''loc_001A5B5C:
        ctx->gpr[0] = vm_read64(ctx->gpr[1] + 0x100);
        {
            const uint32_t result=(uint32_t)ctx->gpr[31];
            const uint32_t least=comet_malloc_ms ? vm_read32((uint64_t)comet_malloc_ms+0x10) : 0u;
            if (result && least && result < least) {
                const uint32_t chunk=result >= 8u ? result-8u : 0u;
                const uint32_t head=chunk ? vm_read32((uint64_t)chunk+4u) : 0u;
                const uint32_t fd=chunk ? vm_read32((uint64_t)chunk+8u) : 0u;
                const uint32_t bk=chunk ? vm_read32((uint64_t)chunk+0xCu) : 0u;
                fprintf(stderr,"[COMET-MALLOC-LOW] caller_lr=0x%08X source=%s request=0x%08X mspace=0x%08X least=0x%08X result=0x%08X chunk=0x%08X head=0x%08X fd=0x%08X bk=0x%08X\n",
                        comet_malloc_caller_lr,comet_malloc_r31_source,comet_malloc_request,comet_malloc_ms,least,result,chunk,head,fd,bk);
                fprintf(stderr,"[COMET-MALLOC-STATE] smallmap=0x%08X treemap=0x%08X dvsize=0x%08X topsize=0x%08X dv=0x%08X top=0x%08X r20=0x%08X r21=0x%08X r25=0x%08X r26=0x%08X r27=0x%08X r29=0x%08X r30=0x%08X r31=0x%08X\n",
                        vm_read32((uint64_t)comet_malloc_ms+0x00),vm_read32((uint64_t)comet_malloc_ms+0x04),
                        vm_read32((uint64_t)comet_malloc_ms+0x08),vm_read32((uint64_t)comet_malloc_ms+0x0C),
                        vm_read32((uint64_t)comet_malloc_ms+0x14),vm_read32((uint64_t)comet_malloc_ms+0x18),
                        (uint32_t)ctx->gpr[20],(uint32_t)ctx->gpr[21],(uint32_t)ctx->gpr[25],(uint32_t)ctx->gpr[26],
                        (uint32_t)ctx->gpr[27],(uint32_t)ctx->gpr[29],(uint32_t)ctx->gpr[30],(uint32_t)ctx->gpr[31]);
            }
        }
        ctx->gpr[3] = ppc_rldicl(ctx->gpr[31], 0, 32);'''
MARKER = "[COMET-MALLOC-LOW]"
EXPECTED_TRACKED_R31_WRITES = 26

def _instrument_r31_writes(func: str) -> tuple[str, int]:
    lines=func.splitlines()
    label="entry"
    tracked=0
    per_label: dict[str,int]={}
    for i,line in enumerate(lines):
        stripped=line.strip()
        if stripped.startswith("loc_") and stripped.endswith(":"):
            label=stripped[:-1]
            continue
        if "ctx->gpr[31] =" not in line or "ctx->gpr[31] = _cs_31;" in line:
            continue
        tracked += 1
        per_label[label]=per_label.get(label,0)+1
        tag=f"{label}#{per_label[label]}"
        indent=line[:len(line)-len(line.lstrip())]
        lines[i]=f'{indent}comet_malloc_r31_source="{tag}"; {stripped}'
    return "\n".join(lines)+("\n" if func.endswith("\n") else ""),tracked

def patch_text(text: str) -> tuple[str, bool]:
    if MARKER in text:
        return text,False
    if text.count(FUNC_ANCHOR) != 1:
        raise ValueError(f"expected one Comet mspace_malloc function anchor, found {text.count(FUNC_ANCHOR)}")
    if text.count(RETURN_ANCHOR) != 1:
        raise ValueError(f"expected one Comet mspace_malloc return anchor, found {text.count(RETURN_ANCHOR)}")
    text=text.replace(FUNC_ANCHOR,FUNC_REPL,1)
    start=text.index("void func_001A5A90(ppu_context* ctx)")
    end=text.find("\nvoid func_",start+1)
    if end < 0:
        end=len(text)
    func=text[start:end]
    func,tracked=_instrument_r31_writes(func)
    if tracked != EXPECTED_TRACKED_R31_WRITES:
        raise ValueError(f"expected {EXPECTED_TRACKED_R31_WRITES} tracked Comet mspace_malloc r31 writes, found {tracked}")
    text=text[:start]+func+text[end:]
    if text.count(RETURN_ANCHOR) != 1:
        raise ValueError("Comet mspace_malloc return anchor drifted during r31 instrumentation")
    text=text.replace(RETURN_ANCHOR,RETURN_REPL,1)
    return text,True

def patch_file(path: Path) -> bool:
    src=path.read_bytes().decode("latin-1")
    out,changed=patch_text(src)
    if changed:
        path.write_bytes(out.encode("latin-1"))
    return changed

def main() -> int:
    ap=argparse.ArgumentParser(description="Trace the exact Comet Crash mspace_malloc producer of a low returned guest pointer")
    ap.add_argument("source",type=Path,help="generated ppu_recomp_*.cpp containing func_001A5A90")
    a=ap.parse_args()
    changed=patch_file(a.source)
    print(f"Comet malloc-source diagnostic patch: {'applied' if changed else 'already present'}")
    return 0

if __name__=="__main__":
    raise SystemExit(main())
