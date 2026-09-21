from __future__ import annotations
import argparse, json, re
from pathlib import Path

FUNC_RE = re.compile(r'void\s+(?P<name>[A-Za-z0-9_]*spu_func_(?P<addr>[0-9A-Fa-f]{8}))\s*\(spu_context\* ctx\)\s*\{')
REF_RE = re.compile(r'\b([A-Za-z0-9_]*spu_func_([0-9A-Fa-f]{8}))\b')
UNSUPPORTED_RE = re.compile(r'spu_unsupported\(0x([0-9A-Fa-f]+),\s*"([^"]+)"\)')

def _functions(src: str):
    matches=list(FUNC_RE.finditer(src)); out={}
    for i,m in enumerate(matches):
        start=m.end(); end=matches[i+1].start() if i+1<len(matches) else len(src)
        out[int(m.group('addr'),16)]={'name':m.group('name'),'body':src[start:end]}
    return out

def audit_text(src: str, entry: int):
    funcs=_functions(src)
    graph={}
    unsupported=[]
    for addr,f in funcs.items():
        refs={int(x[1],16) for x in REF_RE.findall(f['body']) if int(x[1],16) in funcs and int(x[1],16)!=addr}
        graph[addr]=refs
        for m in UNSUPPORTED_RE.finditer(f['body']):
            unsupported.append({'function':addr,'pc':int(m.group(1),16),'mnemonic':m.group(2)})
    reachable=set(); stack=[entry]
    while stack:
        a=stack.pop()
        if a in reachable or a not in funcs: continue
        reachable.add(a); stack.extend(graph.get(a,()))
    ru=[x for x in unsupported if x['function'] in reachable]
    uu=[x for x in unsupported if x['function'] not in reachable]
    return {
        'entry':entry,'functions_total':len(funcs),'reachable_functions':len(reachable),
        'unsupported_total':len(unsupported),'unsupported_reachable':len(ru),
        'unsupported_unreachable':len(uu),'reachable_unsupported':ru,
    }

def audit_file(path: Path, entry: int):
    return audit_text(path.read_text(encoding='utf-8',errors='replace'), entry)

def main() -> int:
    ap=argparse.ArgumentParser(description='Reachability-aware audit of lifted SPU unsupported instructions')
    ap.add_argument('source',type=Path); ap.add_argument('--entry',required=True,type=lambda s:int(s,0)); ap.add_argument('--json',action='store_true')
    a=ap.parse_args(); r=audit_file(a.source,a.entry)
    print(json.dumps(r,indent=2) if a.json else f"reachable={r['reachable_functions']}/{r['functions_total']} unsupported reachable={r['unsupported_reachable']} total={r['unsupported_total']}")
    return 1 if r['unsupported_reachable'] else 0
if __name__=='__main__': raise SystemExit(main())
