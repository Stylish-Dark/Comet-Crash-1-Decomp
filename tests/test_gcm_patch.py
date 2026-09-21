import sys, unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import patch_ps3recomp_gcm as p

C_SAMPLE = r'''
static CellGcmReportData s_report_data[CELL_GCM_MAX_REPORT_COUNT];
static u32 gcm_io2ea(u32 io) { return io; }
/* NID: 0x8572ADE4 */
CellGcmReportData* cellGcmGetReportDataAddress(u32 index)
{
    return &s_report_data[index];
}
/* NID: 0x97FC4B73 */
u64 cellGcmGetTimeStamp(u32 index)
{
    return 0;
}
/* Get report value at index with location */
u32 cellGcmGetReportDataLocation(u32 index, u32 location)
{
    (void)location;

    if (index >= CELL_GCM_MAX_REPORT_COUNT)
        return 0;

    return s_report_data[index].value;
}
void walk(void) {
                } else if (subch == 0 || (subch == 1 && !s1_2d)) {
                    rsx_process_method(&s_state, m, vm_read32(dea));
                    /* NV406E_SET_REFERENCE: queue the fence value for PACED
                     * publication */
}
'''

H_SAMPLE = r'''
#define CELL_GCM_MAX_REPORT_COUNT       256
#define CELL_GCM_REPORT_DATA_SIZE       16
#define CELL_GCM_LOCATION_LOCAL 0
#define CELL_GCM_LOCATION_MAIN 1
/* NID: 0x8572ADE4 */
CellGcmReportData* cellGcmGetReportDataAddress(u32 index);
/* NID: 0x97FC4B73 */
u64 cellGcmGetTimeStamp(u32 index);
'''

class GcmPatchTests(unittest.TestCase):
    def test_adds_comet_report_support(self):
        c, h, changed = p.patch_texts(C_SAMPLE, H_SAMPLE)
        self.assertTrue(changed)
        self.assertIn('#define CELL_GCM_MAX_REPORT_COUNT       2048', h)
        self.assertIn('u32 cellGcmGetReport(u32 type, u32 index)', c)
        self.assertIn('u32 cellGcmGetReport(u32 type, u32 index);', h)
        self.assertIn('0x0E000000u + index * CELL_GCM_REPORT_DATA_SIZE', c)
        self.assertIn('if (m == 0x1800u)', c)
        self.assertIn('u32 value = (type == 1u) ? 1u : 0u;', c)
        self.assertIn('vm_write32(ea + 8u, value);', c)

        c2, h2, changed2 = p.patch_texts(c, h)
        self.assertFalse(changed2)
        self.assertEqual(c2, c)
        self.assertEqual(h2, h)

    def test_upstream_drift_fails(self):
        with self.assertRaises(ValueError):
            p.patch_texts('no report marker', 'no header marker')

if __name__ == '__main__': unittest.main()
