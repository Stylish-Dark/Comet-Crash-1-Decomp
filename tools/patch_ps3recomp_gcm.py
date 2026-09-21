from __future__ import annotations
import argparse
from pathlib import Path

REPORT_COUNT_OLD = '#define CELL_GCM_MAX_REPORT_COUNT       256'
REPORT_COUNT_NEW = '#define CELL_GCM_MAX_REPORT_COUNT       2048'

C_API_ANCHOR = '''/* NID: 0x97FC4B73 */
u64 cellGcmGetTimeStamp(u32 index)
'''
H_API_ANCHOR = '''/* NID: 0x97FC4B73 */
u64 cellGcmGetTimeStamp(u32 index);
'''

C_API_IMPL = '''/* NID: 0x99D397AC */
u32 cellGcmGetReport(u32 type, u32 index)
{
    /* CELL_GCM local report memory has 2048 16-byte slots. Comet Crash uses
     * indices 0x317..0x31A for zcull statistics and reads them through this
     * API after submitting NV4097_GET_REPORT. */
    if (index >= CELL_GCM_MAX_REPORT_COUNT) {
        printf("[cellGcmSys] WARNING: GetReport index %u out of range\\n", index);
        return 0;
    }
    if (type < 1 || type > 5)
        return 0xFFFFFFFFu;
    return s_report_data[index].value;
}

'''

H_API_DECL = '''/* NID: 0x99D397AC */
u32 cellGcmGetReport(u32 type, u32 index);

'''

LOCATION_OLD = '''/* Get report value at index with location */
u32 cellGcmGetReportDataLocation(u32 index, u32 location)
{
    (void)location;

    if (index >= CELL_GCM_MAX_REPORT_COUNT)
        return 0;

    return s_report_data[index].value;
}
'''

LOCATION_NEW = '''/* Get report value at index with location */
u32 cellGcmGetReportDataLocation(u32 index, u32 location)
{
    if (location == CELL_GCM_LOCATION_MAIN) {
        /* Firmware maps MAIN reports through the reserved RSX IO report window:
         * guest EA = gcmIoOffsetToAddress(0x0E000000 + index * 16). Comet uses
         * indices 0x137F..0x1382 here. */
        if (index >= 1024u * 1024u)
            return 0;
        u32 ea = gcm_io2ea(0x0E000000u + index * CELL_GCM_REPORT_DATA_SIZE);
        return ea ? vm_read32(ea + 8u) : 0;
    }

    if (index >= CELL_GCM_MAX_REPORT_COUNT)
        return 0;
    return s_report_data[index].value;
}
'''

FIFO_ANCHOR = '''                } else if (subch == 0 || (subch == 1 && !s1_2d)) {
                    rsx_process_method(&s_state, m, vm_read32(dea));
                    /* NV406E_SET_REFERENCE: queue the fence value for PACED
'''

FIFO_REPLACEMENT = '''                } else if (subch == 0 || (subch == 1 && !s1_2d)) {
                    u32 method_data = vm_read32(dea);
                    if (m == 0x1800u) {
                        /* NV4097_GET_REPORT: high byte = report type, low 24 bits
                         * = byte offset in the currently selected report space.
                         *
                         * Comet uses local offsets 0x3170..0x31A0 and MAIN
                         * offsets 0x137F0..0x13820. The latter are necessarily
                         * outside the 2048-slot local window, so they can be
                         * routed to the standard MAIN report IO window without
                         * guessing any title state.
                         *
                         * We do not yet have real occlusion/zcull counters.
                         * Completing type-1 queries with one visible sample is
                         * the conservative choice (don't hide rendered objects);
                         * statistic types 2..5 complete with zero. */
                        u32 type = method_data >> 24;
                        u32 off = method_data & 0x00FFFFFFu;
                        u32 index = off / CELL_GCM_REPORT_DATA_SIZE;
                        u32 value = (type == 1u) ? 1u : 0u;
                        u64 stamp = get_timestamp_ns();

                        if (index < CELL_GCM_MAX_REPORT_COUNT) {
                            s_report_data[index].timestamp = stamp;
                            s_report_data[index].value = value;
                            s_report_data[index].pad = 0;
                        } else {
                            u32 ea = gcm_io2ea(0x0E000000u + off);
                            if (ea) {
                                vm_write64(ea + 0u, stamp);
                                vm_write32(ea + 8u, value);
                                vm_write32(ea + 12u, 0u);
                            }
                        }
                    } else {
                        rsx_process_method(&s_state, m, method_data);
                    }
                    /* NV406E_SET_REFERENCE: queue the fence value for PACED
'''


def patch_texts(c_src: str, h_src: str) -> tuple[str, str, bool]:
    changed = False

    if REPORT_COUNT_NEW not in h_src:
        if REPORT_COUNT_OLD not in h_src:
            raise ValueError('expected ps3recomp report-count marker not found; upstream changed')
        h_src = h_src.replace(REPORT_COUNT_OLD, REPORT_COUNT_NEW, 1)
        changed = True

    c_done = 'u32 cellGcmGetReport(u32 type, u32 index)' in c_src
    h_done = 'u32 cellGcmGetReport(u32 type, u32 index);' in h_src
    if c_done != h_done:
        raise ValueError('partial cellGcmGetReport API patch found; refusing ambiguous update')
    if not c_done:
        if C_API_ANCHOR not in c_src or H_API_ANCHOR not in h_src:
            raise ValueError('expected ps3recomp cellGcm report API markers not found; upstream changed')
        c_src = c_src.replace(C_API_ANCHOR, C_API_IMPL + C_API_ANCHOR, 1)
        h_src = h_src.replace(H_API_ANCHOR, H_API_DECL + H_API_ANCHOR, 1)
        changed = True

    if LOCATION_NEW not in c_src:
        if LOCATION_OLD not in c_src:
            raise ValueError('expected cellGcmGetReportDataLocation body not found; upstream changed')
        c_src = c_src.replace(LOCATION_OLD, LOCATION_NEW, 1)
        changed = True

    if FIFO_REPLACEMENT not in c_src:
        if FIFO_ANCHOR not in c_src:
            raise ValueError('expected GCM FIFO dispatch marker not found; upstream changed')
        c_src = c_src.replace(FIFO_ANCHOR, FIFO_REPLACEMENT, 1)
        changed = True

    return c_src, h_src, changed


def patch_checkout(root: Path) -> bool:
    c = root / 'libs' / 'video' / 'cellGcmSys.c'
    h = root / 'libs' / 'video' / 'cellGcmSys.h'
    c_src = c.read_text(encoding='utf-8')
    h_src = h.read_text(encoding='utf-8')
    c_out, h_out, changed = patch_texts(c_src, h_src)
    if changed:
        c.write_text(c_out, encoding='utf-8', newline='\n')
        h.write_text(h_out, encoding='utf-8', newline='\n')
    return changed


def main() -> int:
    ap = argparse.ArgumentParser(description='Patch ps3recomp GCM report behavior required by Comet Crash')
    ap.add_argument('ps3recomp', type=Path)
    a = ap.parse_args()
    changed = patch_checkout(a.ps3recomp)
    print(f'Comet GCM reports: {"patched" if changed else "already patched"}')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
