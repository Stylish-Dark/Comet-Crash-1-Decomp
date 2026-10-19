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


## 2026-10-19 — Continuous pilot preview and original input-vector cap

PR #53 Linux graphics, Windows build/package and regression CI succeeded.
Traced pad+150 into F2350: F2DB4..F2E90 caps left-stick X/Z to unit magnitude
and stores commands at player+10/+14. Implemented a portable semantic cap using
hypot/divide; original VMX estimate/refinement bits are not reproduced or tested.
Added opt-in F3 slot0 ship preview; IJKL/left stick move continuously while playing
and the build cursor follows. Speed/heading/bounds are explicit native policy,
not recovered craft physics. Unit release remains at its configured base.
Pause/focus/unplug clear held keys; F2 and human-owner changes reset position.
Review caught arrow navigation bypassing the pilot movement gate; arrows are now
ignored in pilot mode. Added SDL paused-input/arrow suppression and restart tests.
Validation: 62 headless/65 graphics CTests,233 Python tests, five ASan/UBSan cases,
compileall and repository safety. All65536 decoded stick pairs covered natively.
No new original-instruction comparison, original-asset render or physical pad test.
Estimate stays89% toward first playable single-player mission: preview usability
improved, while original craft dynamics remain open. Details: native-pilot.md.


## 2026-10-19 — Original selected craft free-flight recovery

PR #54 native Linux/Windows package and regression CI passed. Found PPU player
snapshot bridge DAD04..DAD78 and FE5D8 10290 consumer, called from12B88. Recovered
selected source velocity target10/40,4.6*delta response or instant response,
optional .7 reduction, old-velocity-first position integration and newY clearing.
NativePilot now retains source velocity and uses this leaf. Pausing/cancel/focus/
unplug explicitly stop it; clipping/heading/mode choice remain native policy.
Private untouched-instruction interpreter comparisons (a2c20108), seed71955:
10,000 cases all8 floatwords exact; outputSHA256
14f1c988338819b370b32bda410450c9f2360593650f69f0cc3ca361500f835f.
Sixteen synthetic/numeric fixtures committed; no proprietary instructions/harness.
Native tests add atomic numeric-domain/overflow rejection, RZ restoration, native
subnormal policy and inertial pilot lifecycle. Full original craft collision,
attachment/action gates and combat/carrying remain open. Validation63CPU/66graphics
CTests,233 Python tests, five sanitizer cases, compileall/safety and code review.
Estimate91% toward first playable single-player mission (89% prior), not fullport.
Details and next boundaries: docs/decomp/native-craft-flight.md.


## 2026-10-19 — Original craft world containment

PR #55 Linux/Windows/native and regression workflows passed. Traced craft10548
through7DD0 to existing7A50/7510 response. Added atomic integrate-contain-respond
composition while retaining the collision-free leaf API. F3 pilot now uses source
boundary motion instead of native clipping; .5 radii,100 restitution/movable and
mode choices remain explicitly native pending original initialization recovery.
Private untouched instruction comparison seed71956:10,000 queries,120,000 float
words plus10,000hitreturns exact,8,442hits; outputSHA256
4c7de4493c7928cc16548051d89a674eac00543ae73a254b103f7c37dcc4573e.
Twenty-four public numeric fixtures; original inputs/harness remain private.
New pilot epsilon-wall assertion failed against old clamp before implementation.
Details: docs/decomp/native-craft-containment.md. Remaining craft constructor,
terrain/entity collision, state/attachment/actions and combat/carrying integration
are explicit. Estimate92% toward first playable single-player mission (91% prior),
not full-port/source coverage. Continue new PR above #55; do not merge the stack.

Validation:63CPU/66graphics CTests,233Python regressions,four focused ASan/UBSan,
compileall/safety/diff checks and clean independent review. An initial existing
keyboard-construction timing assertion failed; unchanged focused and complete
66-test reruns passed. Timing sensitivity is documented, not claimed fixed.


## 2026-10-19 — Original player-craft body initialization

PR #56 regression/native Linux/Windows CI passed. Proved SPU type1dispatch12B88,
PPU CFADC body construction, CFB8C registration and CFBC8 player+220 index link.
Recovered bounded CFAB8..CFB88 slot writes including FECFC, preserving reused
bytes. Radius24/rest100/flagsC006 now derive NativePilot's physical profile;
the assumed .5 radius becomes source-backed .24. Native height/mode/heading and
lifecycle remain policy; body allocation/registration/player/combat links are
not implemented by this initializer. Private decoded-original evaluator seed57019
matched1000cases/all256000bytes; outputSHA256
 dc342eca7a66bebbbc6b328d9fe2bd8784e93b9ca2262ebf0dda41790b83dba0.
Sixteen public full-slot numeric fingerprints. Pilot radius test failed on the
old profile then passed; independent review found no actionable issues.
Details: docs/decomp/native-player-craft-initialization.md. Estimate remains92%
toward first playable single-player mission; no new mission interaction credit.
Continue new PR above #56; original world queries/contacts/actions remain next.

Validation:64headless/67graphics CTests,233Python regressions,five focused
ASan/UBSan,compileall/safety/diff checks pass. Review reran craft/pilot tests.
Graphics first run green; no physical pad/fresh original-asset render claim.
