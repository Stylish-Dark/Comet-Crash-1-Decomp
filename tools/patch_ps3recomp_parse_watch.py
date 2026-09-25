from __future__ import annotations
import argparse
from pathlib import Path

MARKER='[COMET-PARSE-WRITE]'
GLOBALS='extern "C" uint32_t g_ww_lo = 0, g_ww_hi = 0;'
GLOBALS_REPL=r'''extern "C" uint32_t g_ww_lo = 0, g_ww_hi = 0;

/* Comet Crash parse-buffer writer watch.  The title-side diagnostic arms this
 * only while func_00130130 owns its parsed buffer at caller sp+0x84.  Keep it
 * thread-local: multiple guest PPU threads may be active at once. */
static PPU_THREAD_LOCAL uint32_t g_comet_parse_watch = 0;
static PPU_THREAD_LOCAL uint32_t g_comet_parse_expected = 0;
static PPU_THREAD_LOCAL uint32_t g_comet_parse_watch_hits = 0;

extern "C" void comet_parse_watch_arm(uint32_t addr, uint32_t expected)
{
    g_comet_parse_watch = addr;
    g_comet_parse_expected = expected;
    g_comet_parse_watch_hits = 0;
    /* ppu_memory.h's HLE-side write macro gates only on start address, so make
     * its cheap window wide enough to admit any 1/2/4/8-byte store overlapping
     * the watched 32-bit word.  The reporter below rechecks exact overlap. */
    g_ww_lo = addr >= 7 ? addr - 7 : 0;
    g_ww_hi = addr + 4;
    fprintf(stderr, "[COMET-PARSE-WATCH-ARM] slot=0x%08X expected=0x%08X\\n",
            addr, expected);
    fflush(stderr);
}

extern "C" void comet_parse_watch_disarm(void)
{
    if (g_comet_parse_watch)
        fprintf(stderr, "[COMET-PARSE-WATCH-DISARM] slot=0x%08X hits=%u\\n",
                g_comet_parse_watch, g_comet_parse_watch_hits);
    g_comet_parse_watch = 0;
    g_comet_parse_expected = 0;
    g_comet_parse_watch_hits = 0;
    g_ww_lo = g_ww_hi = 0;
}'''

HLE_ENTRY='''extern "C" void ps3_ww_report_inline(uint32_t addr, uint64_t val, int width)
{
    static int n = 0;'''
HLE_REPL=r'''extern "C" void ps3_ww_report_inline(uint32_t addr, uint64_t val, int width)
{
    if (g_comet_parse_watch && addr < g_comet_parse_watch + 4u &&
        addr + (uint32_t)width > g_comet_parse_watch) {
        ++g_comet_parse_watch_hits;
        uint32_t before = vm_read32(g_comet_parse_watch);
        fprintf(stderr,
                "[COMET-PARSE-WRITE-HLE] slot=0x%08X addr=0x%08X value=0x%llX width=%d expected=0x%08X before=0x%08X\\n",
                g_comet_parse_watch, addr, (unsigned long long)val, width,
                g_comet_parse_expected, before);
        fflush(stderr);
    }
    static int n = 0;'''

BARRIER_ENTRY='''static inline void barrier_watch_hit(uint32_t a, uint32_t v, int width, void* ra)
{
    /* PPU_WVAL=<hexvalue>: log every PPU store that WRITES this value, wherever'''
BARRIER_REPL=r'''static inline void barrier_watch_hit(uint32_t a, uint32_t v, int width, void* ra)
{
    if (g_comet_parse_watch && a < g_comet_parse_watch + 4u &&
        a + (uint32_t)width > g_comet_parse_watch) {
        ++g_comet_parse_watch_hits;
        uint32_t before = vm_read32(g_comet_parse_watch);
        uint32_t guest_fn = ppu_prof_resolve_host(ra);
        fprintf(stderr,
                "[COMET-PARSE-WRITE] slot=0x%08X addr=0x%08X value=0x%08X width=%d expected=0x%08X before=0x%08X guest_fn=0x%08X\\n",
                g_comet_parse_watch, a, v, width, g_comet_parse_expected,
                before, guest_fn);
        if (g_comet_parse_watch_hits <= 3) {
            extern PPU_THREAD_LOCAL ppu_context* g_active_ctx;
            extern void ppu_dump_guest_stack(ppu_context*, const char*);
            if (g_active_ctx) ppu_dump_guest_stack(g_active_ctx, "comet-parse-write");
        }
        fflush(stderr);
    }
    /* PPU_WVAL=<hexvalue>: log every PPU store that WRITES this value, wherever'''


def _one(s:str,a:str,b:str,name:str)->str:
    n=s.count(a)
    if n!=1:
        raise ValueError(f'expected one {name} anchor, found {n}')
    return s.replace(a,b,1)


def patch_text(s:str)->tuple[str,bool]:
    if MARKER in s:
        return s,False
    s=_one(s,GLOBALS,GLOBALS_REPL,'write-watch globals')
    s=_one(s,HLE_ENTRY,HLE_REPL,'HLE writer reporter')
    s=_one(s,BARRIER_ENTRY,BARRIER_REPL,'guest writer reporter')
    return s,True


def patch_file(ps3recomp:Path)->bool:
    p=ps3recomp/'runtime'/'ppu'/'ppu_loader.cpp'
    src=p.read_text(encoding='utf-8')
    out,changed=patch_text(src)
    if changed:
        p.write_text(out,encoding='utf-8',newline='\n')
    return changed


def main()->int:
    ap=argparse.ArgumentParser(description='Add Comet parse-slot writer tracing to ps3recomp runtime')
    ap.add_argument('ps3recomp',type=Path)
    a=ap.parse_args()
    changed=patch_file(a.ps3recomp)
    print('Comet parse-slot runtime watch:', 'applied' if changed else 'already present')
    return 0

if __name__=='__main__': raise SystemExit(main())
