import os
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import comet_port
import patch_ps3recomp_vfs as vfs_patch


PPU_SAMPLE = r'''
static void host_path(char* out, size_t cap, const char* guest)
{
    const char* rel = guest;
    static const char* hdd0_root = nullptr; static int hdd0_init = 0;
    if (!hdd0_init) { hdd0_root = getenv("PS3_HDD0_ROOT"); hdd0_init = 1; }
    if (hdd0_root && strncmp(guest, "/dev_hdd0/", 10) == 0) {
        snprintf(out, cap, "%s/%s", hdd0_root, guest + 10);
        for (char* p = out; *p; p++) if (*p == '\\\\') *p = '/';
        return;
    }

    static const char* mounts[] = {
        "/dev_bdvd/", "/app_home/", "/dev_hdd0/", "/dev_hdd1/",
        "/dev_flash/", "/host_root/", "/dev_usb000/", "/dev_usb/"
    };
'''

SYS_SAMPLE = r'''
void sys_fs_translate_path(const char* ps3_path, char* host_path, int host_path_size)
{
    if (g_sys_fs_root[0] == '.' && g_sys_fs_root[1] == '\0') {
        const char* env = getenv("PS3_VFS_ROOT");
        if (env && *env) { strncpy(g_sys_fs_root, env, sizeof(g_sys_fs_root) - 1); g_sys_fs_root[sizeof(g_sys_fs_root)-1] = 0; }
    }

    {
        static const char* hdd0_root = NULL; static int hdd0_init = 0;
        if (!hdd0_init) { hdd0_root = getenv("PS3_HDD0_ROOT"); hdd0_init = 1; }
        if (hdd0_root && strncmp(ps3_path, "/dev_hdd0/", 10) == 0) {
            snprintf(host_path, (size_t)host_path_size, "%s/%s", hdd0_root, ps3_path + 10);
            fs_normalize_sep(host_path);
            return;
        }
    }
'''


class CometBootPathTests(unittest.TestCase):
    def test_run_environment_maps_hdd_title_and_app_home(self):
        with tempfile.TemporaryDirectory() as td:
            title = Path(td) / 'Whatever User Named It'
            (title / 'USRDIR').mkdir(parents=True)
            # run_environment is pure: it should not require a real SFO parser here.
            env = comet_port.runtime_environment(title, title / 'PARAM.SFO')
            self.assertEqual(env['PS3_VFS_ROOT'], str(title.resolve()))
            self.assertEqual(env['PS3_HDD_GAME_ROOT'], str(title.resolve()))
            self.assertEqual(env['PS3_APP_HOME_ROOT'], str((title / 'USRDIR').resolve()))
            self.assertEqual(env['PS3_WORKING_ROOT'], str((title / 'USRDIR').resolve()))
            self.assertEqual(env['PS3_TITLE_ID'], 'NPEB00142')
            self.assertEqual(env['PS3_PARAM_SFO'], str((title / 'PARAM.SFO').resolve()))

    def test_ppu_fs_patch_adds_exact_hdd_apphome_and_working_mappings(self):
        out, changed = vfs_patch.patch_ppu_text(PPU_SAMPLE)
        self.assertTrue(changed)
        for token in ['PS3_HDD_GAME_ROOT', 'PS3_TITLE_ID', 'PS3_APP_HOME_ROOT', 'PS3_WORKING_ROOT']:
            self.assertIn(token, out)
        out2, changed2 = vfs_patch.patch_ppu_text(out)
        self.assertFalse(changed2)
        self.assertEqual(out2, out)

    def test_sys_fs_patch_adds_same_mapping_contract(self):
        out, changed = vfs_patch.patch_sys_text(SYS_SAMPLE)
        self.assertTrue(changed)
        for token in ['PS3_HDD_GAME_ROOT', 'PS3_TITLE_ID', 'PS3_APP_HOME_ROOT', 'PS3_WORKING_ROOT']:
            self.assertIn(token, out)
        out2, changed2 = vfs_patch.patch_sys_text(out)
        self.assertFalse(changed2)
        self.assertEqual(out2, out)

    def test_upstream_drift_fails_loudly(self):
        with self.assertRaises(ValueError):
            vfs_patch.patch_ppu_text('no host_path markers')
        with self.assertRaises(ValueError):
            vfs_patch.patch_sys_text('no sys_fs markers')


if __name__ == '__main__':
    unittest.main()
