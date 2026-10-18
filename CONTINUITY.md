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
