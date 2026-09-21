import sys, unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import patch_ps3recomp_spurs as p

SAMPLE = r"""
#define MAX_JOBCHAINS 32
static struct {
    u32 jc_ea;
    u32 entry_ea;
    u16 size_desc;
    u16 max_grab;
    int run_count;
    volatile long running;
} s_jobchains[MAX_JOBCHAINS];

static s32 jc_register(u32 jc_ea, u32 entry_ea, u16 size_desc, u16 max_grab,
                       const char* who)
{
    for (int i = 0; i < MAX_JOBCHAINS; i++) {
        if (!s_jobchains[i].jc_ea || s_jobchains[i].jc_ea == jc_ea) {
            s_jobchains[i].jc_ea     = jc_ea;
            s_jobchains[i].entry_ea  = entry_ea;
            s_jobchains[i].size_desc = size_desc;
            s_jobchains[i].max_grab  = max_grab;
            s_jobchains[i].run_count = 0;
            return CELL_OK;
        }
    }
    printf("registry full\n");
    return CELL_OK;
}

static void jc_execute(u32 entry_ea, u32 jc_ea, u32 size_desc)
{
    u32 pc = entry_ea, ret_pc = 0;
    int jobs = 0;
    int idle = 0;
    for (;; idle++) {
        if (idle > 4096) break;
        u64 cmd = vm_read64(pc);
        u32 op  = (u32)(cmd & 7);
        u32 ext = (u32)(cmd & 127);

        if (cmd != 0 && op == 0) {                    /* JOB */
            { extern u32 g_spurs_job_ls_handle; g_spurs_job_ls_handle = jc_ea; }
            jc_run_one_job((u32)(cmd & ~7ull), jobs++, size_desc);
            idle = 0;
            jc_signal_done(jc_ea);
            pc += 8; continue;
        }
        if (op == 1) { pc = (u32)(cmd & ~7ull); continue; }   /* RESET_PC */
        if (op == 3) { pc = (u32)(cmd & ~7ull); continue; }   /* NEXT     */
        if (op == 4) { ret_pc = pc + 8; pc = (u32)(cmd & ~7ull); continue; }  /* CALL */
        pc += 8;
    }
}

static DWORD WINAPI jc_thread(LPVOID p)
{
    return 0;
}

static s32 jc_start(u64 jc_ea, const char* who)
{
    static int s_off = -1;
    for (int i = 0; i < MAX_JOBCHAINS; i++) {
        if (s_jobchains[i].jc_ea != (u32)jc_ea) continue;
        if (s_off || !s_jobchains[i].entry_ea) return CELL_OK;
        return CELL_OK;
    }
    return CELL_OK;
}
"""

class SpursPatchTests(unittest.TestCase):
    def test_urgent_fifo_is_added_and_consumed(self):
        out, changed = p.patch_text(SAMPLE)
        self.assertTrue(changed)
        self.assertIn("comet_spurs_add_urgent_command", out)
        self.assertIn("jc_pop_urgent", out)
        self.assertIn("jc_ea + 0x30", out)
        self.assertIn("from_urgent", out)
        self.assertIn("if (!from_urgent) pc += 8", out)
        self.assertIn("ret_pc = from_urgent ? pc : pc + 8", out)
        self.assertIn("vm_write64(jc_ea + 0x30", out)
        self.assertIn("jc_has_urgent((u32)jc_ea)", out)
        self.assertIn("0x80410A11u", out)
        out2, changed2 = p.patch_text(out)
        self.assertFalse(changed2)
        self.assertEqual(out2, out)

    def test_upstream_drift_fails(self):
        with self.assertRaises(ValueError):
            p.patch_text("no jobchain walker here")
