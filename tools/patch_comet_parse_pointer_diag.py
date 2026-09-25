from __future__ import annotations
import argparse
from pathlib import Path

MARKER='[COMET-PARSE-ALLOC]'
PARSE_ENTRY="""void func_0012F590(ppu_context* ctx) {
        uint64_t _cs_23 = ctx->gpr[23];"""
PARSE_ENTRY_REPL="""void func_0012F590(ppu_context* ctx) {
        const uint32_t comet_pp_caller_lr=(uint32_t)ctx->lr;
        const uint32_t comet_pp_out=(uint32_t)ctx->gpr[4];
        uint32_t comet_pp_alloc=0,comet_pp_sp=0;
        int comet_pp_reg_reported=0;
        uint64_t _cs_23 = ctx->gpr[23];"""
PARSE_SP="        vm_write64(ctx->gpr[1] + -0xD0, ctx->gpr[1]); ctx->gpr[1] += -0xD0;"
ALLOC_CALL="""        ctx->lr = 0x0012F698; func_001A80D0(ctx); DRAIN_TRAMPOLINE(ctx);
        /* nop */;"""
R31_CALL="""        ctx->lr = 0x0012F6B8; func_001953CC(ctx); DRAIN_TRAMPOLINE(ctx);
        /* nop */;"""
STORE="        vm_write32(ctx->gpr[27] + 0x0, ctx->gpr[23]);"
CALLER_ENTRY="""void func_00130130(ppu_context* ctx) {
        uint64_t _cs_26 = ctx->gpr[26];"""
CALLER_ENTRY_REPL="""extern \"C\" void comet_parse_watch_arm(uint32_t,uint32_t);
extern \"C\" void comet_parse_watch_disarm(void);

void func_00130130(ppu_context* ctx) {
        uint32_t comet_pc_sp=0,comet_pc_slot=0,comet_pc_expected=0;
        int comet_pc_change_reported=0,comet_pc_sp_reported=0;
        uint64_t _cs_26 = ctx->gpr[26];"""
CALLER_SP="        vm_write64(ctx->gpr[1] + -0x260, ctx->gpr[1]); ctx->gpr[1] += -0x260;"
BASELINE_CALL="        ctx->lr = 0x0013016C; func_0012F590(ctx); DRAIN_TRAMPOLINE(ctx);"
FINAL_FREE="""        ctx->gpr[3] = vm_read32(ctx->gpr[1] + 0x84);
        ctx->lr = 0x00130418; func_001A8178(ctx); DRAIN_TRAMPOLINE(ctx);"""
ZERO_RESULT_BRANCH="        if (((ctx->cr >> 0) & 2)) goto loc_001301B0;"
EARLY_FREE="""        ctx->gpr[3] = vm_read32(ctx->gpr[1] + 0x84);
        ctx->lr = 0x001301A8; func_001A8178(ctx); DRAIN_TRAMPOLINE(ctx);"""
PARSE_R23_SITES={'0x0012F6E4':'00195950','0x0012F6FC':'00194868','0x0012F704':'000E70B4','0x0012F718':'0019FDEC','0x0012FC7C':'0012F264'}
CALLER_SITES={
'0x001301E8':'0015470C','0x001301F4':'00011BCC','0x00130204':'0001C7D4','0x00130214':'0001C6E4',
'0x001302F0':'0001C810','0x00130304':'0001C124','0x00130314':'0001C6E4','0x00130320':'00011488',
'0x0013032C':'0015465C','0x00130334':'000E70B4','0x00130340':'0015470C','0x0013034C':'00011BCC',
'0x0013035C':'0001C6E4','0x00130378':'0001DB64','0x00130398':'0001DB64','0x001303B0':'0001DB64',
'0x001303C4':'0001DB64','0x001303DC':'0001DB64','0x001303EC':'0001C6E4','0x001303F8':'00011488',
'0x00130404':'0015465C','0x0013040C':'000E70B4','0x00130440':'00194830','0x00130488':'0001CFD0',
'0x00130498':'0001C6E4','0x001304A4':'00011488','0x001304B0':'0015465C','0x001304B8':'000E70B4',
'0x001304C4':'0015470C','0x001304D0':'00011BCC','0x001304E0':'0001C6E4','0x0013050C':'0001C124',
'0x00130528':'0001DB64','0x00130534':'0002D868','0x00130548':'0001C124'}

def _one(s,a,b,name):
    n=s.count(a)
    if n!=1: raise ValueError(f'expected one {name} anchor, found {n}')
    return s.replace(a,b,1)

def _replace_func(text,addr,fn):
    start=text.find(f"void func_{addr}(ppu_context* ctx)")
    if start<0: raise ValueError(f"missing func_{addr}")
    end=text.find("\nvoid func_",start+1)
    if end<0: end=len(text)
    return text[:start]+fn(text[start:end])+text[end:]

def patch_text(text:str)->tuple[str,bool]:
    if MARKER in text: return text,False
    def parse(body):
        body=_one(body,PARSE_ENTRY,PARSE_ENTRY_REPL,'parse entry')
        body=_one(body,PARSE_SP,PARSE_SP+'\n        comet_pp_sp=(uint32_t)ctx->gpr[1];','parse stack')
        body=_one(body,ALLOC_CALL,ALLOC_CALL+r'''
        comet_pp_alloc=(uint32_t)ctx->gpr[3];
        fprintf(stderr,"[COMET-PARSE-ALLOC] caller_lr=0x%08X out=0x%08X sp=0x%08X alloc=0x%08X size=0x%08X\n",comet_pp_caller_lr,comet_pp_out,comet_pp_sp,comet_pp_alloc,(uint32_t)ctx->gpr[28]);''','alloc')
        body=_one(body,R31_CALL,R31_CALL+r'''
        if(!comet_pp_reg_reported&&comet_pp_alloc&&(uint32_t)ctx->gpr[31]!=comet_pp_alloc){
            fprintf(stderr,"[COMET-PARSE-REG-CLOBBER] site=0x0012F6B8 reg=r31 expected=0x%08X got=0x%08X sp=0x%08X expected_sp=0x%08X\n",comet_pp_alloc,(uint32_t)ctx->gpr[31],(uint32_t)ctx->gpr[1],comet_pp_sp); comet_pp_reg_reported=1;}''','post alloc')
        for site,func in PARSE_R23_SITES.items():
            call=f'        ctx->lr = {site}; func_{func}(ctx); DRAIN_TRAMPOLINE(ctx);'
            repl=call+f'''
        if(!comet_pp_reg_reported&&comet_pp_alloc&&(uint32_t)ctx->gpr[23]!=comet_pp_alloc){{
            fprintf(stderr,"[COMET-PARSE-REG-CLOBBER] site={site} reg=r23 expected=0x%08X got=0x%08X sp=0x%08X expected_sp=0x%08X\\n",comet_pp_alloc,(uint32_t)ctx->gpr[23],(uint32_t)ctx->gpr[1],comet_pp_sp); comet_pp_reg_reported=1;}}'''
            body=_one(body,call,repl,'r23 '+site)
        body=_one(body,STORE,r'''        if(!comet_pp_reg_reported&&comet_pp_alloc&&(uint32_t)ctx->gpr[23]!=comet_pp_alloc){
            fprintf(stderr,"[COMET-PARSE-REG-CLOBBER] site=0x0012FC7C reg=r23 phase=pre-store expected=0x%08X got=0x%08X sp=0x%08X expected_sp=0x%08X\\n",comet_pp_alloc,(uint32_t)ctx->gpr[23],(uint32_t)ctx->gpr[1],comet_pp_sp); comet_pp_reg_reported=1;}
        fprintf(stderr,"[COMET-PARSE-STORE] out=0x%08X entry_out=0x%08X alloc=0x%08X r23=0x%08X old=0x%08X sp=0x%08X expected_sp=0x%08X\n",(uint32_t)ctx->gpr[27],comet_pp_out,comet_pp_alloc,(uint32_t)ctx->gpr[23],vm_read32(ctx->gpr[27]),(uint32_t)ctx->gpr[1],comet_pp_sp);
        vm_write32(ctx->gpr[27] + 0x0, ctx->gpr[23]);''','store')
        return body
    def caller(body):
        body=_one(body,CALLER_ENTRY,CALLER_ENTRY_REPL,'caller entry')
        body=_one(body,CALLER_SP,CALLER_SP+'\n        comet_pc_sp=(uint32_t)ctx->gpr[1];','caller stack')
        body=_one(body,BASELINE_CALL,BASELINE_CALL+r'''
        comet_pc_slot=comet_pc_sp+0x84u; comet_pc_expected=vm_read32(comet_pc_slot);
        fprintf(stderr,"[COMET-PARSE-OUT] slot=0x%08X value=0x%08X sp=0x%08X current_sp=0x%08X\n",comet_pc_slot,comet_pc_expected,comet_pc_sp,(uint32_t)ctx->gpr[1]);''','baseline')
        body=_one(body,ZERO_RESULT_BRANCH,ZERO_RESULT_BRANCH+'\n        comet_parse_watch_arm(comet_pc_slot,comet_pc_expected);','watch arm')
        body=_one(body,EARLY_FREE,'        comet_parse_watch_disarm();\n'+EARLY_FREE,'early free disarm')
        for site,func in CALLER_SITES.items():
            call=f'        ctx->lr = {site}; func_{func}(ctx); DRAIN_TRAMPOLINE(ctx);'
            repl=call+f'''
        if(!comet_pc_sp_reported&&(uint32_t)ctx->gpr[1]!=comet_pc_sp){{fprintf(stderr,"[COMET-PARSE-SP-CHANGE] site={site} expected_sp=0x%08X got_sp=0x%08X slot=0x%08X slot_now=0x%08X\\n",comet_pc_sp,(uint32_t)ctx->gpr[1],comet_pc_slot,vm_read32(comet_pc_slot)); comet_pc_sp_reported=1;}}
        if(!comet_pc_change_reported&&vm_read32(comet_pc_slot)!=comet_pc_expected){{fprintf(stderr,"[COMET-PARSE-SLOT-CHANGE] site={site} callee=0x{func} slot=0x%08X expected=0x%08X got=0x%08X sp=0x%08X\\n",comet_pc_slot,comet_pc_expected,vm_read32(comet_pc_slot),(uint32_t)ctx->gpr[1]); comet_pc_change_reported=1;}}'''
            body=_one(body,call,repl,'watch '+site)
        body=_one(body,FINAL_FREE,r'''        if(!comet_pc_sp_reported&&(uint32_t)ctx->gpr[1]!=comet_pc_sp){fprintf(stderr,"[COMET-PARSE-SP-CHANGE] site=0x00130418 phase=pre-free expected_sp=0x%08X got_sp=0x%08X slot=0x%08X slot_now=0x%08X\\n",comet_pc_sp,(uint32_t)ctx->gpr[1],comet_pc_slot,vm_read32(comet_pc_slot)); comet_pc_sp_reported=1;}
        if(!comet_pc_change_reported&&vm_read32(comet_pc_slot)!=comet_pc_expected){fprintf(stderr,"[COMET-PARSE-SLOT-CHANGE] site=0x00130418 callee=0x001A8178 phase=pre-free slot=0x%08X expected=0x%08X got=0x%08X sp=0x%08X\\n",comet_pc_slot,comet_pc_expected,vm_read32(comet_pc_slot),(uint32_t)ctx->gpr[1]); comet_pc_change_reported=1;}
        fprintf(stderr,"[COMET-PARSE-FINAL] slot=0x%08X expected=0x%08X slot_now=0x%08X sp=0x%08X expected_sp=0x%08X sp84=0x%08X\n",comet_pc_slot,comet_pc_expected,vm_read32(comet_pc_slot),(uint32_t)ctx->gpr[1],comet_pc_sp,vm_read32(ctx->gpr[1]+0x84));
        comet_parse_watch_disarm();
        ctx->gpr[3] = vm_read32(ctx->gpr[1] + 0x84);
        ctx->lr = 0x00130418; func_001A8178(ctx); DRAIN_TRAMPOLINE(ctx);''','final')
        return body
    return _replace_func(_replace_func(text,'0012F590',parse),'00130130',caller),True

def patch_file(path:Path)->bool:
    src=path.read_bytes().decode('latin-1'); out,changed=patch_text(src)
    if changed:path.write_bytes(out.encode('latin-1'))
    return changed

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('source',type=Path); a=ap.parse_args()
    print("Comet parse-pointer diagnostic patch:", "applied" if patch_file(a.source) else "already present")
    return 0
if __name__=='__main__': raise SystemExit(main())
