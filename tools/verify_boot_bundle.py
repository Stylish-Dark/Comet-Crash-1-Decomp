from __future__ import annotations

import argparse
import json
from pathlib import Path

from boot_triage import summarize_file
from comet_port import sha256_file


MATCH_FIELDS = (
    "last_boot_stage",
    "first_signal",
    "triage_signal",
    "host_exit_code",
    "first_frame_presented",
    "suspected_subsystem",
    "memalign_origin",
    "memalign_signal",
    "malloc_source",
    "malloc_signal",
    "boot_outcome",
)


def adjacent_log_path(summary_path: Path) -> Path:
    name=summary_path.name
    suffix=".summary.json"
    if not name.endswith(suffix):
        raise ValueError(f"boot summary must end with {suffix}: {summary_path}")
    return summary_path.with_name(name[:-len(suffix)]+".txt")


def verify_bundle(summary_path: Path, *, require_verified_provenance: bool=True) -> dict[str,object]:
    summary_path=summary_path.resolve()
    errors: list[str]=[]
    if not summary_path.is_file():
        raise FileNotFoundError(summary_path)
    try:
        summary=json.loads(summary_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as e:
        raise ValueError(f"invalid boot summary JSON: {e}") from e

    if summary.get("summary_schema_version") != 1:
        errors.append(f"unsupported summary schema {summary.get('summary_schema_version')!r}")

    log_path=adjacent_log_path(summary_path)
    if not log_path.is_file():
        errors.append(f"adjacent text log missing: {log_path}")
        return {
            "ok":False,
            "summary_path":str(summary_path),
            "log_path":str(log_path),
            "errors":errors,
        }

    expected_log_sha=summary.get("boot_log_sha256")
    actual_log_sha=sha256_file(log_path)
    if expected_log_sha != actual_log_sha:
        errors.append(
            f"boot log SHA-256 mismatch: summary={expected_log_sha}, actual={actual_log_sha}"
        )

    derived=summarize_file(log_path)
    for field in MATCH_FIELDS:
        if summary.get(field) != derived.get(field):
            errors.append(
                f"{field} mismatch: summary={summary.get(field)!r}, derived={derived.get(field)!r}"
            )

    provenance=summary.get("build_provenance")
    if not isinstance(provenance,dict) or provenance.get("schema_version") != 2:
        errors.append("missing or unsupported build provenance")
    else:
        artifact=provenance.get("native_executable") or {}
        if not artifact.get("sha256") or artifact.get("size") is None:
            errors.append("build provenance does not bind the native executable")
        hle=provenance.get("hle_coverage") or {}
        if hle.get("imports") != 171 or hle.get("covered_imports") != 171 or hle.get("missing") != 0:
            errors.append(f"build provenance HLE coverage is incomplete: {hle}")

    verification=summary.get("provenance_verification") or {}
    if require_verified_provenance and verification.get("status") != "verified":
        errors.append(
            f"native provenance was not verified at launch: {verification.get('status')!r}"
        )

    if not summary.get("elf_sha256"):
        errors.append("ELF SHA-256 missing from boot summary")

    return {
        "ok":not errors,
        "summary_path":str(summary_path),
        "log_path":str(log_path),
        "boot_log_sha256":actual_log_sha,
        "boot_outcome":derived.get("boot_outcome"),
        "suspected_subsystem":derived.get("suspected_subsystem"),
        "provenance_status":verification.get("status"),
        "errors":errors,
    }


def main() -> int:
    ap=argparse.ArgumentParser(
        description="Verify a Comet Crash boot .summary.json against its adjacent text log and build provenance"
    )
    ap.add_argument("summary",type=Path)
    ap.add_argument(
        "--allow-unverified-provenance",
        action="store_true",
        help="accept a bundle produced with the explicit --allow-unprovenanced diagnostic override",
    )
    a=ap.parse_args()
    report=verify_bundle(
        a.summary,
        require_verified_provenance=not a.allow_unverified_provenance,
    )
    print(json.dumps(report,indent=2,sort_keys=True))
    return 0 if report["ok"] else 1


if __name__=="__main__":
    raise SystemExit(main())
