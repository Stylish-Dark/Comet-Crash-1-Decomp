from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

BOOT_STAGE_PREFIX = "[boot-stage] "

# Ordered from the most specific failure signatures to broader subsystem hints.
CATEGORY_PATTERNS: tuple[tuple[str, tuple[str, ...]], ...] = (
    ("vm/ppu", (
        "vm allocation failed",
        "ppu_load_elf failed",
        "unresolved indirect",
        "ppu dispatch",
        "ppu exception",
    )),
    ("vfs", (
        "[vfs]",
        "sys_fs",
        "cellfs",
        "/dev_hdd0",
        "/app_home",
        "vfs",
        "file not found",
        "path translation",
    )),
    ("hle", (
        "[hle] unimplemented",
        "unimplemented nid",
        "unresolved import",
        "hle dispatch",
    )),
    ("gcm/resc/rsx", (
        "d3d12 init failed",
        "cellgcm",
        "cellresc",
        "[rsx]",
        "rsx ",
        "gcm ",
        "resc ",
        "flip queue",
    )),
    ("spurs/spu", (
        "cellspurs",
        "[spu]",
        "unsupported spu",
        "spu ",
        "jobchain",
        "job chain",
    )),
    ("synchronization", (
        "hotread",
        "deadlock",
        "spin wait",
        "spin-wait",
        "semaphore",
        "mutex",
        "condition variable",
    )),
    ("audio", (
        "cellaudio",
        "multistream",
        "mp3",
        "audio ",
        "audio]",
    )),
    ("input", (
        "cellpad",
        "xinput",
        "mouse ",
        "keyboard",
        "controller",
    )),
)

SIGNAL_PATTERNS: tuple[str, ...] = (
    "[crash]",
    "[watchdog]",
    "[HLE] UNIMPLEMENTED",
    "unresolved indirect",
    "HOTREAD",
    "unsupported SPU",
    "VM allocation failed",
    "ppu_load_elf failed",
    "D3D12 init FAILED",
)

SUMMARY_RE = re.compile(r"^#\s*([a-z_]+)=(.*)$")


def classify_signal(signal: str | None) -> tuple[str, str]:
    """Return (subsystem, rationale) for one concrete runtime signal.

    Generic crash/watchdog lines deliberately remain ``unknown`` because they are
    symptoms, not subsystem evidence. This keeps the triage tool from inventing a
    blocker before the log contains a concrete signature.
    """
    if not signal:
        return "unknown", "no concrete failure signal was captured"

    text = signal.strip()
    lower = text.lower()
    for category, needles in CATEGORY_PATTERNS:
        for needle in needles:
            if needle in lower:
                return category, f"matched {needle!r} in first concrete signal"

    if "[watchdog]" in lower:
        return "unknown", "watchdog confirms a hang but does not identify its subsystem"
    if "[crash]" in lower:
        return "unknown", "native crash captured without a subsystem-specific signature"
    return "unknown", "signal did not match a known subsystem signature"


def _detect_first_signal(line: str) -> str | None:
    lower = line.lower()
    for marker in SIGNAL_PATTERNS:
        if marker.lower() in lower:
            return line.strip()
    return None


def summarize_lines(lines: list[str]) -> dict[str, object]:
    last_stage: str | None = None
    first_signal: str | None = None
    first_specific_signal: str | None = None
    host_exit_code: int | None = None
    footer: dict[str, str] = {}

    for raw in lines:
        line = raw.rstrip("\r\n")
        if line.startswith(BOOT_STAGE_PREFIX):
            last_stage = line[len(BOOT_STAGE_PREFIX):].strip()
        detected = _detect_first_signal(line)
        if detected is not None:
            if first_signal is None:
                first_signal = detected
            category, _ = classify_signal(detected)
            if category != "unknown" and first_specific_signal is None:
                first_specific_signal = detected

        m = SUMMARY_RE.match(line)
        if m:
            footer[m.group(1)] = m.group(2).strip()

    # Prefer the runner's explicit footer because it is written after streaming
    # completes and is therefore the authoritative summary when present.
    if footer.get("last_boot_stage") and footer["last_boot_stage"] != "<none>":
        last_stage = footer["last_boot_stage"]
    if footer.get("first_signal") and footer["first_signal"] != "<none>":
        first_signal = footer["first_signal"]
    if footer.get("triage_signal") and footer["triage_signal"] != "<none>":
        first_specific_signal = footer["triage_signal"]
    if "host_exit_code" in footer:
        try:
            host_exit_code = int(footer["host_exit_code"], 0)
        except ValueError:
            host_exit_code = None

    triage_signal = first_specific_signal or first_signal
    subsystem, rationale = classify_signal(triage_signal)
    first_frame_presented = any(
        line.strip() == f"{BOOT_STAGE_PREFIX}first guest frame presented"
        for line in lines
    )

    return {
        "last_boot_stage": last_stage,
        "first_signal": first_signal,
        "triage_signal": triage_signal,
        "host_exit_code": host_exit_code,
        "first_frame_presented": first_frame_presented,
        "suspected_subsystem": subsystem,
        "rationale": rationale,
    }


def summarize_file(path: Path) -> dict[str, object]:
    return summarize_lines(path.read_text(encoding="utf-8", errors="replace").splitlines())


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Classify a Comet Crash native boot log without guessing beyond the evidence"
    )
    ap.add_argument("log", type=Path)
    ap.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    args = ap.parse_args()

    report = summarize_file(args.log)
    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        print(f"last boot stage:      {report['last_boot_stage'] or '<none>'}")
        print(f"first signal:         {report['first_signal'] or '<none>'}")
        print(f"triage signal:        {report['triage_signal'] or '<none>'}")
        print(f"host exit code:       {report['host_exit_code'] if report['host_exit_code'] is not None else '<unknown>'}")
        print(f"first frame presented:{' yes' if report['first_frame_presented'] else ' no'}")
        print(f"suspected subsystem:  {report['suspected_subsystem']}")
        print(f"rationale:            {report['rationale']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
