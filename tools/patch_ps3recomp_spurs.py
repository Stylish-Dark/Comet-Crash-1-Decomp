from __future__ import annotations
import argparse
from pathlib import Path

INSERT_AFTER = '} s_jobchains[MAX_JOBCHAINS];\n'
REGISTER_MARK = '            s_jobchains[i].run_count = 0;\n'
EXEC_START = 'static void jc_execute(u32 entry_ea, u32 jc_ea, u32 size_desc)\n{\n'
EXEC_END = '\nstatic DWORD WINAPI jc_thread(LPVOID p)\n'

URGENT_BLOCK = r'''

/* Comet Crash uses cellSpursAddUrgentCommand before RunJobChain for both
 * direct JOB commands and RESET_PC commands that jump into a dynamic command
 * ring. The generic host walker originally ignored CellSpursJobChain::urgentCmds
 * entirely, so those calls returned through an unresolved import and the work
 * never reached the lifted SPU job runner.
 *
 * Keep the real ABI-visible FIFO in the guest CellSpursJobChain at +0x30.
 * Four big-endian u64 slots are shifted exactly like Sony's jobchain policy
 * module / RPCS3. A tiny host lock serialises the PPU enqueue with the host
 * walker dequeue; this is a port-runtime bridge, not guest-visible state. */
static volatile long s_comet_urgent_lock = 0;

static void comet_urgent_lock(void)
{
    while (_InterlockedCompareExchange(&s_comet_urgent_lock, 1, 0) != 0) Sleep(0);
}
static void comet_urgent_unlock(void)
{
    (void)_InterlockedCompareExchange(&s_comet_urgent_lock, 0, 1);
}

/* Called by the Comet-specific context HLE override for NID 0x17001000.
 * Returns the official cellSpurs JOB-domain errors used by this API. */
s32 comet_spurs_add_urgent_command(u64 jc_ea64, u64 new_cmd)
{
    const u32 jc_ea = (u32)jc_ea64;
    enum {
        COMET_SPURS_JOB_ERROR_NULL_POINTER = (s32)0x80410A11u,
        COMET_SPURS_JOB_ERROR_ALIGN        = (s32)0x80410A10u,
        COMET_SPURS_JOB_ERROR_INVAL        = (s32)0x80410A02u,
        COMET_SPURS_JOB_ERROR_BUSY         = (s32)0x80410A0Au,
    };
    if (!jc_ea) return COMET_SPURS_JOB_ERROR_NULL_POINTER;
    if (jc_ea & 0x7Fu) return COMET_SPURS_JOB_ERROR_ALIGN;

    int known = 0;
    for (int i = 0; i < MAX_JOBCHAINS; i++)
        if (s_jobchains[i].jc_ea == jc_ea) { known = 1; break; }
    if (!known) return COMET_SPURS_JOB_ERROR_INVAL;

    comet_urgent_lock();
    for (int i = 0; i < 4; i++) {
        const u32 ea = jc_ea + 0x30u + (u32)i * 8u;
        if (vm_read64(ea) == 0) {
            vm_write64(ea, new_cmd);
            comet_urgent_unlock();
            { static int n = 0; if (n++ < 12)
                printf("[cellSpurs] AddUrgentCommand(jc=0x%08X cmd=0x%016llX slot=%d)\\n",
                       jc_ea, (unsigned long long)new_cmd, i); }
            return CELL_OK;
        }
    }
    comet_urgent_unlock();
    return COMET_SPURS_JOB_ERROR_BUSY;
}

static int jc_has_urgent(u32 jc_ea)
{
    int yes;
    comet_urgent_lock();
    yes = vm_read64(jc_ea + 0x30u) != 0;
    comet_urgent_unlock();
    return yes;
}

static u64 jc_pop_urgent(u32 jc_ea)
{
    u64 cmd;
    comet_urgent_lock();
    cmd = vm_read64(jc_ea + 0x30u);
    if (cmd) {
        vm_write64(jc_ea + 0x30u, vm_read64(jc_ea + 0x38u));
        vm_write64(jc_ea + 0x38u, vm_read64(jc_ea + 0x40u));
        vm_write64(jc_ea + 0x40u, vm_read64(jc_ea + 0x48u));
        vm_write64(jc_ea + 0x48u, 0);
    }
    comet_urgent_unlock();
    return cmd;
}
'''


def patch_text(src: str) -> tuple[str, bool]:
    if 'comet_spurs_add_urgent_command' in src and 'jc_pop_urgent' in src:
        return src, False
    if INSERT_AFTER not in src or EXEC_START not in src or EXEC_END not in src or REGISTER_MARK not in src:
        raise ValueError('expected ps3recomp jobchain markers not found; upstream changed')

    out = src.replace(INSERT_AFTER, INSERT_AFTER + URGENT_BLOCK, 1)
    out = out.replace(
        REGISTER_MARK,
        REGISTER_MARK +
        '            /* Real CellSpursJobChain creation starts with an empty urgent FIFO. */\n'
        '            for (int _u = 0; _u < 4; _u++) vm_write64(jc_ea + 0x30u + (u32)_u * 8u, 0);\n',
        1,
    )

    start = out.index(EXEC_START)
    end = out.index(EXEC_END, start)
    body = out[start:end]
    old = '        u64 cmd = vm_read64(pc);\n'
    if old not in body:
        raise ValueError('jobchain command fetch marker changed')
    body = body.replace(old, '''        u64 cmd = jc_pop_urgent(jc_ea);\n        const int from_urgent = cmd != 0;\n        if (!from_urgent) {\n            if (!pc) return;\n            cmd = vm_read64(pc);\n        }\n''', 1)
    old_call = '        if (op == 4) { ret_pc = pc + 8; pc = (u32)(cmd & ~7ull); continue; }  /* CALL */\n'
    if old_call not in body:
        raise ValueError('jobchain CALL marker changed')
    body = body.replace(old_call,
        '        if (op == 4) { ret_pc = from_urgent ? pc : pc + 8; pc = (u32)(cmd & ~7ull); continue; }  /* CALL */\n', 1)
    # Every sequential advance belongs only to a command fetched from the normal
    # PC stream. An urgent command is out-of-band; consuming it must leave pc at
    # the normal stream position unless the command itself redirects pc.
    body = body.replace('            pc += 8; continue;\n', '            if (!from_urgent) pc += 8; continue;\n')
    body = body.replace('            pc += 8; continue; }', '            if (!from_urgent) pc += 8; continue; }')
    body = body.replace('        pc += 8;\n', '        if (!from_urgent) pc += 8;\n')
    out = out[:start] + body + out[end:]

    start_gate = '        if (s_off || !s_jobchains[i].entry_ea) return CELL_OK;\n'
    if start_gate not in out:
        raise ValueError('jobchain start gate marker changed')
    out = out.replace(start_gate,
        '        if (s_off || (!s_jobchains[i].entry_ea && !jc_has_urgent((u32)jc_ea))) return CELL_OK;\n', 1)
    return out, True


def patch_file(path: Path) -> bool:
    src = path.read_text(encoding='utf-8')
    out, changed = patch_text(src)
    if changed:
        path.write_text(out, encoding='utf-8', newline='\n')
    return changed


def main() -> int:
    ap = argparse.ArgumentParser(description='Patch ps3recomp SPURS jobchain urgent-command support for Comet Crash')
    ap.add_argument('ps3recomp', type=Path)
    a = ap.parse_args()
    p = a.ps3recomp / 'libs' / 'spurs' / 'cellSpurs.c'
    changed = patch_file(p)
    print(f'{p}: {"patched" if changed else "already patched"}')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
