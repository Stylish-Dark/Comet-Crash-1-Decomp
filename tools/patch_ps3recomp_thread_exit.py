from __future__ import annotations

import argparse
from pathlib import Path

OLD = "if (allow && s_exit_armed) { s_exit_armed = 0; longjmp(s_exit_jmp, 1); }"
MARKER = "_endthreadex(0); /* Comet Windows thread-exit fix */"

NEW = """if (allow && s_exit_armed) {
            s_exit_armed = 0;
#ifdef _WIN32
            /* This host thread was created with _beginthreadex().  On Windows,
             * longjmp() across deep recompiled frames can force the OS unwinder
             * through generated COFF function tables and fail with
             * STATUS_BAD_FUNCTION_TABLE (0xC00000FF).  sys_ppu_thread_exit has
             * already stored status and signalled joiners above, so terminate
             * the corresponding CRT thread directly. */
            _endthreadex(0); /* Comet Windows thread-exit fix */
#else
            longjmp(s_exit_jmp, 1);
#endif
        }"""

def patch_text(text: str) -> tuple[str, bool]:
    if MARKER in text:
        return text, False
    count=text.count(OLD)
    if count != 1:
        raise ValueError(f"expected one pinned ps3recomp sys_ppu_thread_exit unwind site, found {count}; upstream changed")
    return text.replace(OLD,NEW), True

def patch_file(ps3recomp: Path) -> bool:
    path=ps3recomp/'runtime'/'syscalls'/'sys_ppu_thread.c'
    if not path.is_file():
        raise FileNotFoundError(path)
    src=path.read_text(encoding='utf-8')
    out,changed=patch_text(src)
    if changed:
        path.write_text(out,encoding='utf-8',newline='\n')
    return changed

def main() -> int:
    ap=argparse.ArgumentParser(description="Patch pinned ps3recomp Windows PPU thread exit to avoid STATUS_BAD_FUNCTION_TABLE")
    ap.add_argument('ps3recomp',type=Path)
    a=ap.parse_args()
    changed=patch_file(a.ps3recomp)
    print(f"Comet Windows PPU thread-exit patch: {'applied' if changed else 'already present'}")
    return 0

if __name__=='__main__':
    raise SystemExit(main())
