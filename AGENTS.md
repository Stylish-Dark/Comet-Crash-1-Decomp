# Faithful Comet Crash recovery contract

Confirmed by the project owner on 2026-10-20. This governs all subsequent work.

## Deliverable

Recover Comet Crash NPEB00142 v1.00 into readable C/C++ and adapt that recovered
game to native Windows PC, including mouse/keyboard controls and controller
support. Preserve the original gameplay calculations, decisions, state changes,
update order, physics, collisions, AI, economy, missions, progression and content.
A similar game or a playable preview with replacement rules is not completion.

The original PS3 instruction encoding will not be the PC instruction encoding.
Preserve instruction semantics and observable game behavior; document necessary
platform adaptations. Do not silently replace an unknown original branch,
calculation, scheduler or system with a plausible implementation. Unknown behavior
remains unrecovered. Do not promise whole-game exactness based on isolated tests.

## Work selection and stopping rule

- Focus now on decompilation and recovery of original game behavior. Choose the
  largest missing original gameplay dependency that can be completed coherently
  within a bounded work unit. Explain which missing original path it closes.
- Recover connected caller/state/update/commit paths, not an indefinite sequence
  of isolated math leaves. Existing leaves and preview code are useful scaffolding.
- Do not expand preview AI, synthetic missions, invented balance or guessed rules
  as a substitute for recovering their original counterparts.
- Defer cosmetic polish and minor refinements unless a concrete discrepancy blocks
  faithful recovery or integration. Do not revisit passing work without a reason.
- Verify the changed path and its material dependencies proportionately. Source
  evidence and focused comparisons matter; repeated broad validation does not
  constitute additional recovery. Recheck after relevant changes or failures.
- A bounded unit is done when the original path is implemented, its material
  behavior is checked, and remaining uncertainty is explicit. Then move forward.

## Evidence and progress

- Retain original addresses, source evidence and supported names. Distinguish
  recovered logic, incomplete recovery, PC interfaces and preview replacements.
- Output-bit matching proves only the specified inputs/outputs and scope. An
  interpreter is not original hardware; exclusions and numeric fallbacks remain
  unresolved fidelity items. A compiler-matching original executable is a
  separate metric and is not currently established by this project.
- The prior 92% estimate and all earlier subjective capability percentages are
  withdrawn. They measure neither full-port completion nor remaining effort.
- Do not publish a new overall percentage or time estimate without an explicit
  scope, defensible coverage/effort basis and uncertainty. Function counts alone
  do not measure remaining work; 3,409 PPU functions excludes SPU and integration.
- Report original behavior recovered, connected end to end, remaining blockers,
  substitutions and unresolved fidelity differences. Documentation work is not
  additional game-code recovery. Test counts are not completion percentages.

## Existing work

Preserve useful recovered code, provenance and preview tooling. Do not delete or
rewrite it merely because priorities changed. Keep preview behavior visibly
separate from claims about the original game. Do not merge the existing PR stack
or alter the final architecture merely to make the preview seem more complete.

Read STATE.md, WORK_QUEUE.md and docs/decomp/fidelity-status.md before selecting
work. PROJECT_PLAN.md and DECISIONS.md carry the same governing objective.
Historical entries below current notices are evidence of past work, not authority
for old percentages or superseded priorities.
