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
        "[comet-memalign-",
        "[comet-malloc-low]",
        "[comet-parse-reg-clobber]",
        "[comet-parse-sp-change]",
        "[comet-parse-slot-change]",
        "[comet-parse-write]",
        "[comet-parse-write-hle]",
        "[comet-alloc-corruption]",
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
        "[d3d12] error:",
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
    "[D3D12] ERROR:",
    "D3D12 init FAILED",
    "[COMET-MEMALIGN-MALLOC-LOW]",
    "[COMET-MEMALIGN-CORE-LOW]",
    "[COMET-MEMALIGN-WRAPPER-LOW]",
    "[COMET-MALLOC-LOW]",
    "[COMET-PARSE-REG-CLOBBER]",
    "[COMET-PARSE-SP-CHANGE]",
    "[COMET-PARSE-WRITE]",
    "[COMET-PARSE-WRITE-HLE]",
    "[COMET-PARSE-SLOT-CHANGE]",
    "[COMET-ALLOC-CORRUPTION]",
)

MEMALIGN_MARKERS: tuple[tuple[str, str], ...] = (
    ("backing-malloc", "[COMET-MEMALIGN-MALLOC-LOW]"),
    ("alignment-core", "[COMET-MEMALIGN-CORE-LOW]"),
    ("wrapper-return", "[COMET-MEMALIGN-WRAPPER-LOW]"),
)

MALLOC_LOW_RE = re.compile(r"\[COMET-MALLOC-LOW\].*?\bsource=([^\s]+)")
PARSE_CORRUPTION_RE = re.compile(r"\[COMET-PARSE-(REG-CLOBBER|SP-CHANGE|SLOT-CHANGE)\].*?\bsite=(0x[0-9A-Fa-f]+)")
PARSE_WRITE_RE = re.compile(r"\[COMET-PARSE-WRITE\].*?\bguest_fn=(0x[0-9A-Fa-f]+)")
PARSE_WRITE_HLE_RE = re.compile(r"\[COMET-PARSE-WRITE-HLE\]")
SUMMARY_RE = re.compile(r"^#\s*([a-z_]+)=(.*)$")


def extract_malloc_source(line: str) -> str | None:
    m=MALLOC_LOW_RE.search(line)
    return m.group(1) if m else None


def extract_parse_corruption(line: str) -> tuple[str, str] | None:
    m=PARSE_CORRUPTION_RE.search(line)
    return (m.group(1).lower(), m.group(2).upper()) if m else None


def extract_parse_write(line: str) -> tuple[str, str] | None:
    m=PARSE_WRITE_RE.search(line)
    if m:
        return ("guest-write", m.group(1).upper())
    if PARSE_WRITE_HLE_RE.search(line):
        return ("hle-write", "HLE")
    return None


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


def derive_outcome(*, first_frame_presented: bool, host_exit_code: int | None,
                   timed_out: bool = False, interrupted: str | None = None,
                   first_signal: str | None = None) -> str:
    if interrupted:
        return "interrupted"
    if timed_out:
        return "timed-out"
    if host_exit_code is None:
        return "incomplete"
    if first_frame_presented:
        return "clean-visible-exit" if host_exit_code == 0 else "visible-output-then-failure"
    if host_exit_code == 0:
        return "clean-exit-before-frame"
    if first_signal:
        return "failure-before-frame"
    return "nonzero-exit-before-frame"


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
    memalign_hits: dict[str, str] = {}
    malloc_source: str | None = None
    malloc_signal: str | None = None
    parse_corruption_kind: str | None = None
    parse_corruption_site: str | None = None
    parse_corruption_signal: str | None = None
    parse_write_kind: str | None = None
    parse_write_function: str | None = None
    parse_write_signal: str | None = None

    for raw in lines:
        line = raw.rstrip("\r\n")
        m = SUMMARY_RE.match(line)
        if m:
            footer[m.group(1)] = m.group(2).strip()
            continue

        if line.startswith(BOOT_STAGE_PREFIX):
            last_stage = line[len(BOOT_STAGE_PREFIX):].strip()
        lower = line.lower()
        write_hit=extract_parse_write(line)
        if write_hit is not None and parse_write_kind is None:
            parse_write_kind,parse_write_function=write_hit
            parse_write_signal=line.strip()
        parse_hit=extract_parse_corruption(line)
        if parse_hit is not None and parse_corruption_kind is None:
            parse_corruption_kind,parse_corruption_site=parse_hit
            parse_corruption_signal=line.strip()
        source=extract_malloc_source(line)
        if source is not None and malloc_source is None:
            malloc_source=source
            malloc_signal=line.strip()
        for origin, marker in MEMALIGN_MARKERS:
            if marker.lower() in lower and origin not in memalign_hits:
                memalign_hits[origin] = line.strip()

        detected = _detect_first_signal(line)
        if detected is not None:
            if first_signal is None:
                first_signal = detected
            category, _ = classify_signal(detected)
            if category != "unknown" and first_specific_signal is None:
                first_specific_signal = detected

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
    memalign_origin: str | None = None
    memalign_signal: str | None = None
    for origin, _ in MEMALIGN_MARKERS:
        if origin in memalign_hits:
            memalign_origin = origin
            memalign_signal = memalign_hits[origin]
            break
    first_frame_presented = any(
        line.strip() == f"{BOOT_STAGE_PREFIX}first guest frame presented"
        for line in lines
    )
    if footer.get("first_frame_presented") in ("true", "false"):
        first_frame_presented = footer["first_frame_presented"] == "true"
    timed_out = footer.get("timed_out") == "true"
    interrupted = footer.get("interrupted")
    if interrupted in (None, "", "<none>"):
        interrupted = None
    outcome = derive_outcome(
        first_frame_presented=first_frame_presented,
        host_exit_code=host_exit_code,
        timed_out=timed_out,
        interrupted=interrupted,
        first_signal=first_signal,
    )

    return {
        "last_boot_stage": last_stage,
        "first_signal": first_signal,
        "triage_signal": triage_signal,
        "host_exit_code": host_exit_code,
        "first_frame_presented": first_frame_presented,
        "suspected_subsystem": subsystem,
        "rationale": rationale,
        "memalign_origin": memalign_origin,
        "memalign_signal": memalign_signal,
        "malloc_source": malloc_source,
        "malloc_signal": malloc_signal,
        "parse_corruption_kind": parse_corruption_kind,
        "parse_corruption_site": parse_corruption_site,
        "parse_corruption_signal": parse_corruption_signal,
        "parse_write_kind": parse_write_kind,
        "parse_write_function": parse_write_function,
        "parse_write_signal": parse_write_signal,
        "boot_outcome": outcome,
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
        print(f"memalign origin:       {report['memalign_origin'] or '<none>'}")
        print(f"memalign signal:       {report['memalign_signal'] or '<none>'}")
        print(f"malloc source:         {report['malloc_source'] or '<none>'}")
        print(f"malloc signal:         {report['malloc_signal'] or '<none>'}")
        print(f"parse corruption:      {report['parse_corruption_kind'] or '<none>'}")
        print(f"parse corruption site: {report['parse_corruption_site'] or '<none>'}")
        print(f"parse writer:          {report['parse_write_kind'] or '<none>'}")
        print(f"parse writer function: {report['parse_write_function'] or '<none>'}")
        print(f"boot outcome:         {report['boot_outcome']}")
        print(f"rationale:            {report['rationale']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
