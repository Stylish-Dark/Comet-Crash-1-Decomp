from __future__ import annotations
import argparse, hashlib, json, struct
from pathlib import Path

EM_PPC64=21; EM_SPU=23; PT_LOAD=1; PT_PRX_STUB=0x60000002

def u16(b,o): return struct.unpack_from('>H',b,o)[0]
def u32(b,o): return struct.unpack_from('>I',b,o)[0]
def u64(b,o): return struct.unpack_from('>Q',b,o)[0]

def parse_elf64_header(b: bytes) -> dict:
    if len(b)<64 or b[:4]!=b'\x7fELF' or b[4]!=2 or b[5]!=2: raise ValueError('expected ELF64 big-endian')
    return {'machine':u16(b,18),'entry':u64(b,24),'phoff':u64(b,32),'shoff':u64(b,40),'phentsize':u16(b,54),'phnum':u16(b,56),'shentsize':u16(b,58),'shnum':u16(b,60),'shstrndx':u16(b,62)}

def phdrs(b,h):
    out=[]
    for i in range(h['phnum']):
        o=h['phoff']+i*h['phentsize']
        if o+56>len(b): break
        out.append({'type':u32(b,o),'flags':u32(b,o+4),'offset':u64(b,o+8),'vaddr':u64(b,o+16),'filesz':u64(b,o+32),'memsz':u64(b,o+40)})
    return out

def vaddr_to_offset(addr, ph):
    for p in ph:
        if p['type']==PT_LOAD and p['vaddr']<=addr<p['vaddr']+p['filesz']:
            return p['offset']+(addr-p['vaddr'])
    return None


def read_vaddr(b: bytes, ph: list[dict], addr: int, size: int) -> bytes | None:
    off=vaddr_to_offset(addr,ph)
    if off is None or off+size>len(b): return None
    return b[off:off+size]

def cstr_vaddr(b: bytes, ph: list[dict], addr: int, cap: int=256) -> str | None:
    off=vaddr_to_offset(addr,ph)
    if off is None: return None
    end=min(len(b),off+cap); z=b.find(b'\0',off,end)
    if z<0: z=end
    raw=b[off:z]
    try: return raw.decode('ascii')
    except UnicodeDecodeError: return raw.decode('ascii','replace')

def parse_imports(b: bytes, ph: list[dict]) -> list[dict]:
    # Cell LV2's PT_PRX_STUB header points to a table of 32-bit PRX module
    # import descriptors. Each descriptor supplies the library name, NID table,
    # and trampoline/stub-address table.
    special=[x for x in ph if x['type']==PT_PRX_STUB]
    if not special: return []
    h=read_vaddr(b,ph,special[0]['vaddr'],0x20)
    if h is None or len(h)<0x20: return []
    begin=u32(h,24); end=u32(h,28)
    if not begin or end<=begin: return []

    desc=[]; cur=begin
    while cur<end:
        head=read_vaddr(b,ph,cur,0x1C)
        if head is None or not head: break
        size=head[0]
        if size<0x1C or cur+size>end: break
        desc.append({
            'addr':cur, 'size':size, 'nfunc':u16(head,6),
            'name':u32(head,0x10), 'nids':u32(head,0x14),
            'stubs':u32(head,0x18),
        })
        cur+=size

    imports=[]
    for i,d in enumerate(desc):
        n=d['nfunc']
        # A few SDK-generated descriptors leave nfunc zero. When the next
        # descriptor's tables are contiguous, their pointer delta gives the
        # actual number of entries.
        if n==0 and i+1<len(desc):
            nd=desc[i+1]
            counts=[]
            if d['nids'] and nd['nids']>d['nids']: counts.append((nd['nids']-d['nids'])//4)
            if d['stubs'] and nd['stubs']>d['stubs']: counts.append((nd['stubs']-d['stubs'])//4)
            counts=[x for x in counts if 0<x<0x10000]
            if counts: n=min(counts)
        if n<=0: continue
        lib=cstr_vaddr(b,ph,d['name']) or f'library@0x{d["name"]:08X}'
        for j in range(n):
            nb=read_vaddr(b,ph,d['nids']+j*4,4); sb=read_vaddr(b,ph,d['stubs']+j*4,4)
            if nb is None or sb is None: break
            imports.append({'library':lib,'nid':f'0x{u32(nb,0):08X}','stub':f'0x{u32(sb,0):08X}'})
    return imports

def scan_direct_branch_calls(b: bytes, ph: list[dict], targets: set[int]) -> dict[int,list[int]]:
    found={t:[] for t in targets}
    for seg in ph:
        if seg['type']!=PT_LOAD or not (seg.get('flags',0)&1): continue
        lo=int(seg['offset']); n=int(seg['filesz']); va=int(seg['vaddr'])
        hi=min(len(b),lo+n)
        for off in range(lo,hi-3,4):
            insn=u32(b,off)
            # PPC b/bl: primary opcode 18. Restrict to relative-link branches
            # (AA=0, LK=1), the normal direct function-call encoding.
            if (insn>>26)!=18 or (insn&3)!=1: continue
            disp=insn&0x03FFFFFC
            if disp&0x02000000: disp-=0x04000000
            pc=va+(off-lo); target=(pc+disp)&0xFFFFFFFF
            if target in found: found[target].append(pc)
    return {k:v for k,v in found.items() if v}

def scan_spu_images(b: bytes, ph: list[dict]) -> list[dict]:
    out=[]; start=0
    while True:
        o=b.find(b'\x7fELF\x01\x02',start)
        if o<0: break
        start=o+1
        if o+52>len(b) or u16(b,o+18)!=EM_SPU: continue
        phoff=u32(b,o+28); shoff=u32(b,o+32); phents=u16(b,o+42); phnum=u16(b,o+44); shents=u16(b,o+46); shnum=u16(b,o+48)
        end=max(52, shoff+shents*shnum)
        for i in range(phnum):
            p=o+phoff+i*phents
            if p+32<=len(b): end=max(end,u32(b,p+4)+u32(b,p+16))
        for i in range(shnum):
            s=o+shoff+i*shents
            if s+40<=len(b) and u32(b,s+4)!=8: end=max(end,u32(b,s+16)+u32(b,s+20))
        end=min(end,len(b)-o); blob=b[o:o+end]
        va=None
        for p in ph:
            if p['offset']<=o<p['offset']+p['filesz']: va=p['vaddr']+(o-p['offset']); break
        strings=[]
        for token in blob.split(b'\0'):
            if 8<=len(token)<=180 and all(32<=c<127 for c in token):
                s=token.decode('ascii','ignore')
                if 'spu' in s.lower() or 'mp3' in s.lower() or '/home/' in s: strings.append(s)
        out.append({'file_offset':f'0x{o:08X}','ppu_vaddr':f'0x{va:08X}' if va is not None else None,'size':end,'sha256':hashlib.sha256(blob).hexdigest(),'entry':f'0x{u32(b,o+24):08X}','strings':strings[:12]})
    return out

def parse_opd(b: bytes, ph: list[dict]) -> dict:
    # PS3 PPU function descriptors are compact 8-byte pairs: a 32-bit code
    # address followed by the module TOC. The surrounding file is ELF64, but
    # guest virtual addresses remain 32-bit. Find the densest contiguous run
    # whose code pointers land in executable PT_LOADs and whose TOCs are mapped.
    def is_exec_vaddr(addr: int) -> bool:
        for seg in ph:
            if seg['type']==PT_LOAD and (seg.get('flags',0)&1) and seg['vaddr']<=addr<seg['vaddr']+seg['filesz']:
                return True
        return False

    candidates=[]
    for seg in ph:
        if seg['type']!=PT_LOAD or not (seg.get('flags',0)&2):
            continue
        lo=int(seg['offset']); hi=min(len(b),lo+int(seg['filesz']))
        run=[]
        for o in range((lo+7)&~7, hi-7, 8):
            code=u32(b,o); toc=u32(b,o+4)
            valid=is_exec_vaddr(code) and vaddr_to_offset(toc,ph) is not None
            if valid:
                run.append((o,code,toc))
            else:
                if len(run)>=32: candidates.append(run)
                run=[]
        if len(run)>=32: candidates.append(run)
    if not candidates:
        return {'descriptor_count':0,'unique_function_count':0}
    run=max(candidates,key=len)
    return {
        'file_offset':f'0x{run[0][0]:08X}',
        'descriptor_count':len(run),
        'unique_function_count':len({x[1] for x in run}),
        'first_code':f'0x{run[0][1]:08X}',
        'module_toc':f'0x{run[0][2]:08X}',
    }

def probe_elf(path: Path) -> dict:
    b=path.read_bytes(); h=parse_elf64_header(b)
    ph=phdrs(b,h)
    if h['machine']!=EM_PPC64: raise ValueError(f'expected PPC64 machine {EM_PPC64}, got {h["machine"]}')
    imports=parse_imports(b,ph)
    targets={int(x['stub'],16) for x in imports}
    calls=scan_direct_branch_calls(b,ph,targets)
    by_library={}
    for x in imports: by_library[x['library']]=by_library.get(x['library'],0)+1
    for x in imports:
        target=int(x['stub'],16)
        if target in calls: x['direct_calls']=[f'0x{pc:08X}' for pc in calls[target]]
    return {'path':str(path),'size':len(b),'sha256':hashlib.sha256(b).hexdigest(),'entry':f'0x{h["entry"]:08X}','program_headers':ph,'opd':parse_opd(b,ph),'firmware_imports':{'count':len(imports),'library_count':len(by_library),'by_library':dict(sorted(by_library.items())),'imports':imports},'spu_images':scan_spu_images(b,ph)}

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('elf',type=Path); ap.add_argument('-o','--output',type=Path)
    a=ap.parse_args(); r=probe_elf(a.elf); s=json.dumps(r,indent=2)
    if a.output: a.output.parent.mkdir(parents=True,exist_ok=True); a.output.write_text(s+'\n')
    print(s)
if __name__=='__main__': main()
