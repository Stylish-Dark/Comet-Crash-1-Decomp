# Full-port completion assessment — 2026-10-21

**Planning estimate: about 25% complete. Plausible range: 15–35%.**

100% means the complete original NPEB00142 v1.00 game faithfully recovered and
integrated as a usable native Windows game, including mouse-capable controls,
original gameplay/content/progression, presentation, audio and persistence.
A replacement skirmish or one playable mission is not the denominator.

This is an engineering judgment of completed versus remaining work, rounded to
five percentage points. It is not measured decompilation coverage, byte matching,
a sum of passing tests, or a statement that 25% of calendar time has elapsed.
The range expresses uncertainty, not a statistical confidence interval. The
available evidence does not support a reliable remaining-hours or weeks forecast.

## Operational completion map — 2026-10-22

[PROJECT_PLAN.md](../../PROJECT_PLAN.md) now contains stable work-item IDs,
current statuses, dependency order, completion criteria and runnable gates.
G0 preview foundation exists; G1 original player loop and G2 first original
mission are unaccepted. Use that map to show concrete before/after progress.
The map itself adds no game completion. Unequal task counts cannot be converted
into a remaining-time or overall-effort percentage. Calibrate item forecasts
against actual comparable source/integration work as the plan is executed.

## Basis

The assessment uses STATE.md, the subsystem recovery documents and the current
fidelity inventory. It gives most of the remaining effort to original gameplay
and its integration: these contain unknown branches and dependencies, whereas
several data-loading and native-platform foundations already exist. It gives no
original-game recovery credit to the replacement opponent or synthetic missions.
It does not assign equal weight to functions or count a partial subsystem as done.

| Work group | Evidence available | Remaining effort judgment |
| --- | --- | --- |
| Recovery tools, original data and native foundation | Pinned executable/function metadata, source tools, original loaders, renderers, native builds and preview input | A substantial foundation exists, but complete platform integration remains |
| Original arena simulation | Source-backed construction, production, selected ground movement/contact/capture/death, selected weapons, craft creation and shared finalization | Considerable real code exists; complete callers, ordering, other entity families and fidelity exclusions remain substantial |
| Full original player gameplay | Selected flight/facing and connected creation/commit boundaries | Most controller state, action, carrying and complete input integration remains |
| Original AI, missions and campaign | Reference evidence and replacement preview scaffolding | Most original behavior remains; preview implementations are not credited |
| Complete presentation, audio, saves and release integration | Models/textures and selected renderer/platform work | Much remains, including menus/HUD/effects, complete sound/persistence and whole-game compatibility |

Those findings support an early-to-middle implementation state with **most work
still ahead**, rather than anything near completion. About one quarter is a
coarse planning judgment; there is not yet a reliable measured work breakdown
that makes a tighter percentage defensible. Discovery in the unrecovered systems
could move this baseline significantly. Do not invent precise weights to make
this look calculated.

## Updating the estimate

Keep the full deliverable fixed. Reassess when a major original gameplay system
becomes integrated, or discovery changes the remaining workload materially.
Small commits can be valuable without moving a rounded overall percentage. Report
what became possible and the next blocking path, not fractional increases chosen
to suggest speed. Explain revisions explicitly. This baseline replaces the
withdrawn 92% claim; it does not mean completed work was undone.

Track measured PPU/SPU recovery coverage separately only after a source inventory
and status ledger establish a fixed denominator. The known 3,409 PPU functions do
not include all SPU work or native integration and cannot produce this percentage.
