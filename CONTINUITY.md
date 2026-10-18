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
