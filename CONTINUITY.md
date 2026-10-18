# CONTINUITY

This file is retained for compatibility with older project handoffs.

**Authoritative current state: read `STATE.md` first.**

Then read:
1. `WORK_QUEUE.md` for the bounded next work unit;
2. `PROJECT_PLAN.md` for architecture and phases;
3. `DECISIONS.md` for settled choices;
4. `SESSION_LOG.md` for chronological history.

Do not reconstruct project state from chat history when these repository files are available.


## 2026-10-19 — Autonomous native preview opponent

Continued published PR #50 at 55b6591, preserving unfinished old worktrees and
unrelated Windows script line-ending diffs. Added explicit `opponent OWNER`
configuration and simulation-owned one-second decisions: ordinary factory
placement, default fighter selection, quota-limited production and held release.
Funds, queue, route rejection, allocation, scene projection and checkpoint
restoration use existing paths. Added public uncommanded base-depletion scenario.

Review reproduced continued production after defeat and new attacks targeting
dead bases in three-team encounters; regression tests now cover both. Added
route-rejected chokepoint placement, alternate retry and cursor restoration.
This is native preview policy, not recovered original enemy AI. Native movement rebinds also exclude dead/inactive/allied targets. Original
campaign/economy/control rules remain open.
Details: docs/decomp/native-opponent.md. Estimate 86% toward a first playable
single-player mission (84% prior), not full-port/source-coverage percentage.


## 2026-10-19 — Interactive skirmish launch and dynamic base arrival

PR #51's native graphics/Windows and regression workflows passed. Continued its
published head 6a28a322. Added map-independent native skirmish bootstrap, visible
base markers, strict transactional validation, --skirmish-config CLI, packaged
Windows launcher/config and human/AI input separation. Viewer selects the first
human and initializes/restores its base cursor. Added SDL-driven battle/F2 tests.

The exact supplied contacts/cannon-enabled config exposed a latent ground adapter
bug: position W was read from packed entity metadata at +0C, so target-cell
steering produced nonzero W and orientation rejected all entry motion. XYZ+zeroW
fix restores dynamic base arrival; the public encounter reaches depletion.
See docs/decomp/native-skirmish.md for validation and explicit native boundaries.
Estimate 88% toward a first playable single-player mission (86% prior), not source
coverage/full-port completion. Original craft/aircraft, adaptive AI, additional
weapons and original economy/mission/campaign/presentation remain open.


## 2026-10-19 — Recovered stick normalization and native gamepad controls

Continued PR #52 after its native/Windows and regression workflows succeeded.
Recovered the four-axis scalar normalization sequence at PPU F1728..F1868 from
pinned original ELF. Private subset interpreter and public all256-input fixture
agree, including deadzone boundaries, positive endpoint and float rounding.
SDL first-controller input now drives existing native build/factory/deployment
and mission controls, with hotplug, neutral rearming, separate keyboard/pad
release holds and controller overlay hints. Native grid/action policy is named
explicitly; original craft/aircraft and full pad bindings remain open.
Validation: 61 CPU /64 graphics CTests,233 Python tests, five ASan/UBSan checks,
compileall and repository safety. New virtual SDL test covers build/type/kind,
X/R2 release, factory cycling, focus/hot-unplug interruption and held restart.
No proprietary bytes or private oracle tool committed. Details and hashes:
docs/decomp/native-gamepad.md. Estimate89% toward first playable single-player
mission (88% prior), not full-port/source coverage. Next: original craft consumer.
