import sys, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import patch_ps3recomp_parse_watch as p

class ParseWatchRuntimePatchTests(unittest.TestCase):
    def fixture(self):
        return '\n'.join([
            'extern "C" uint32_t g_ww_lo = 0, g_ww_hi = 0;',
            '',
            'extern "C" void ps3_ww_report_inline(uint32_t addr, uint64_t val, int width)',
            '{',
            '    static int n = 0;',
            '    if (n++ >= 64) return;',
            '}',
            '',
            'static inline void barrier_watch_hit(uint32_t a, uint32_t v, int width, void* ra)',
            '{',
            '    /* PPU_WVAL=<hexvalue>: log every PPU store that WRITES this value, wherever',
            '     * it lands. */',
            '}',
        ])+'\n'

    def test_adds_thread_local_dynamic_parse_watch(self):
        out,changed=p.patch_text(self.fixture())
        self.assertTrue(changed)
        self.assertIn('g_comet_parse_watch',out)
        self.assertIn('PPU_THREAD_LOCAL uint32_t g_comet_parse_watch',out)
        self.assertIn('extern "C" void comet_parse_watch_arm',out)
        self.assertIn('extern "C" void comet_parse_watch_disarm',out)
        self.assertIn('[COMET-PARSE-WRITE]',out)
        self.assertIn('[COMET-PARSE-WRITE-HLE]',out)
        self.assertIn('ppu_prof_resolve_host(ra)',out)
        self.assertIn('ppu_dump_guest_stack',out)
        out2,changed2=p.patch_text(out)
        self.assertFalse(changed2)
        self.assertEqual(out2,out)

    def test_drift_fails_loudly(self):
        with self.assertRaises(ValueError):
            p.patch_text('upstream changed')

if __name__=='__main__': unittest.main()
