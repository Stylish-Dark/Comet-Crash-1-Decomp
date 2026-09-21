import sys, unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import patch_ps3recomp_resc as p

SAMPLE = r'''
s32 cellRescCreateInterlaceTable(void* buf, float ea, u32 tableLen, s32 depth)
{
    (void)ea;
    (void)depth;

    printf("[cellResc] CreateInterlaceTable(len=%u, depth=%d)\\n", tableLen, depth);

    if (!buf)
        return (s32)CELL_RESC_ERROR_BAD_ARGUMENT;

    /* Fill with simple linear interpolation weights */
    float* table = (float*)buf;
    for (u32 i = 0; i < tableLen; i++)
        table[i] = (float)i / (float)(tableLen > 1 ? tableLen - 1 : 1);

    return CELL_OK;
}
'''

class RescPatchTests(unittest.TestCase):
    def test_interlace_table_writes_guest_big_endian(self):
        out, changed = p.patch_text(SAMPLE)
        self.assertTrue(changed)
        self.assertNotIn('float* table = (float*)buf;', out)
        self.assertIn('const u32 table_ea = (u32)(uintptr_t)buf;', out)
        self.assertIn('memcpy(&bits, &weight, sizeof(bits));', out)
        self.assertIn('vm_write32(table_ea + i * 4u, bits);', out)
        out2, changed2 = p.patch_text(out)
        self.assertFalse(changed2)
        self.assertEqual(out2, out)

    def test_upstream_drift_fails(self):
        with self.assertRaises(ValueError):
            p.patch_text('no resc interlace implementation here')

if __name__ == '__main__': unittest.main()
