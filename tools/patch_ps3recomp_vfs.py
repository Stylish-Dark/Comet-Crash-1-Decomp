from __future__ import annotations
import argparse
from pathlib import Path

PPU_MARK = '    const char* rel = guest;\n'
SYS_MARK = '''    if (g_sys_fs_root[0] == '.' && g_sys_fs_root[1] == '\\0') {\n        const char* env = getenv("PS3_VFS_ROOT");\n        if (env && *env) { strncpy(g_sys_fs_root, env, sizeof(g_sys_fs_root) - 1); g_sys_fs_root[sizeof(g_sys_fs_root)-1] = 0; }\n    }\n'''

PPU_BLOCK = r'''
    /* Comet/HDD-title host mappings.  A dumped PSN title is commonly supplied
     * as an arbitrary folder containing PARAM.SFO + USRDIR rather than as a
     * literal dev_hdd0/game/<TITLE_ID> tree.  Keep the guest-visible paths
     * authentic while letting the host folder be named anything. */
    const char* comet_app_home = getenv("PS3_APP_HOME_ROOT");
    if (comet_app_home && *comet_app_home &&
        (strcmp(guest, "/app_home") == 0 || strncmp(guest, "/app_home/", 10) == 0)) {
        const char* tail = guest[9] ? guest + 10 : "";
        snprintf(out, cap, "%s%s%s", comet_app_home, *tail ? "/" : "", tail);
        for (char* p = out; *p; p++) if (*p == '\\') *p = '/';
        return;
    }

    const char* comet_hdd_game = getenv("PS3_HDD_GAME_ROOT");
    const char* comet_title_id = getenv("PS3_TITLE_ID");
    if (comet_hdd_game && *comet_hdd_game && comet_title_id && *comet_title_id) {
        char prefix[128];
        snprintf(prefix, sizeof prefix, "/dev_hdd0/game/%s", comet_title_id);
        size_t n = strlen(prefix);
        if (strncmp(guest, prefix, n) == 0 && (guest[n] == '\0' || guest[n] == '/')) {
            const char* tail = guest[n] == '/' ? guest + n + 1 : "";
            snprintf(out, cap, "%s%s%s", comet_hdd_game, *tail ? "/" : "", tail);
            for (char* p = out; *p; p++) if (*p == '\\') *p = '/';
            return;
        }
    }

    const char* comet_working = getenv("PS3_WORKING_ROOT");
    if (comet_working && *comet_working && guest[0] != '/') {
        snprintf(out, cap, "%s/%s", comet_working, guest);
        for (char* p = out; *p; p++) if (*p == '\\') *p = '/';
        return;
    }
'''

SYS_BLOCK = r'''

    /* Same HDD-title contract as runtime/ppu/ppu_fs.cpp.  Keep raw LV2 fs
     * syscalls and imported cellFs calls pointed at the same host files. */
    {
        const char* app_home = getenv("PS3_APP_HOME_ROOT");
        if (app_home && *app_home &&
            (strcmp(ps3_path, "/app_home") == 0 || strncmp(ps3_path, "/app_home/", 10) == 0)) {
            const char* tail = ps3_path[9] ? ps3_path + 10 : "";
            snprintf(host_path, (size_t)host_path_size, "%s%s%s", app_home, *tail ? "/" : "", tail);
            fs_normalize_sep(host_path);
            return;
        }

        const char* hdd_game = getenv("PS3_HDD_GAME_ROOT");
        const char* title_id = getenv("PS3_TITLE_ID");
        if (hdd_game && *hdd_game && title_id && *title_id) {
            char prefix[128];
            snprintf(prefix, sizeof prefix, "/dev_hdd0/game/%s", title_id);
            size_t n = strlen(prefix);
            if (strncmp(ps3_path, prefix, n) == 0 && (ps3_path[n] == '\0' || ps3_path[n] == '/')) {
                const char* tail = ps3_path[n] == '/' ? ps3_path + n + 1 : "";
                snprintf(host_path, (size_t)host_path_size, "%s%s%s", hdd_game, *tail ? "/" : "", tail);
                fs_normalize_sep(host_path);
                return;
            }
        }

        const char* working = getenv("PS3_WORKING_ROOT");
        if (working && *working && ps3_path[0] != '/') {
            snprintf(host_path, (size_t)host_path_size, "%s/%s", working, ps3_path);
            fs_normalize_sep(host_path);
            return;
        }
    }
'''


def patch_ppu_text(src: str) -> tuple[str, bool]:
    if 'PS3_HDD_GAME_ROOT' in src and 'PS3_APP_HOME_ROOT' in src and 'PS3_WORKING_ROOT' in src:
        return src, False
    if 'static void host_path(char* out, size_t cap, const char* guest)' not in src or PPU_MARK not in src:
        raise ValueError('expected ppu_fs host_path markers not found; upstream changed')
    return src.replace(PPU_MARK, PPU_MARK + PPU_BLOCK, 1), True


def patch_sys_text(src: str) -> tuple[str, bool]:
    if 'PS3_HDD_GAME_ROOT' in src and 'PS3_APP_HOME_ROOT' in src and 'PS3_WORKING_ROOT' in src:
        return src, False
    if 'void sys_fs_translate_path(const char* ps3_path, char* host_path, int host_path_size)' not in src or SYS_MARK not in src:
        raise ValueError('expected sys_fs translation markers not found; upstream changed')
    return src.replace(SYS_MARK, SYS_MARK + SYS_BLOCK, 1), True


def patch_file(path: Path, fn) -> bool:
    src = path.read_text(encoding='utf-8')
    out, changed = fn(src)
    if changed:
        path.write_text(out, encoding='utf-8', newline='\n')
    return changed


def patch_checkout(root: Path) -> dict[str, bool]:
    return {
        'ppu_fs': patch_file(root/'runtime'/'ppu'/'ppu_fs.cpp', patch_ppu_text),
        'sys_fs': patch_file(root/'runtime'/'syscalls'/'sys_fs.c', patch_sys_text),
    }


def main() -> int:
    ap=argparse.ArgumentParser(description='Patch ps3recomp VFS for extracted HDD/PSN titles')
    ap.add_argument('ps3recomp', type=Path)
    a=ap.parse_args()
    for name, changed in patch_checkout(a.ps3recomp).items():
        print(f'{name}: {"patched" if changed else "already patched"}')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
