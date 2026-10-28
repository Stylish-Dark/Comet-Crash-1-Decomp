# Original health-display producer implementation plan

Goal: close the3940 first-display stop in the registered9A70 ->10290 ->6578
player update, preserving source allocation failure and dirty record bytes.
Architecture: recover3940/1E480 into a shared producer using the existing FIFO
request pool; pass original cached kind/flags/offset and reused two-record scratch
through finalization. Source transport remains explicit; no guessed scheduler.
Spec: AGENTS.md, PROJECT_PLAN.md P1/P4/P5/P6 and original FE5D8 instructions.

- [x] Recover both3940 model families105/106 and101/102, reserve2+1024,
  identity/phase/reference metadata and untouched bytes. Compare complete records
  and final body against original instructions, including full pool and existing flag.
- [x] Connect to shared finalization and registered scene. Missing transport stays
  explicit; full pool commits health with7F0; success sets7F1 once. Stage publication
  until authoritative bank commit succeeds; reject invalid/cell-full atomically.
- [x] Exercise a real multi-update attachment sequence starting with7F0 and real
  display publication. Document source boundaries, independently review, build
  native targets and publish a stacked checkpoint without merging prior PRs.

Verification is focused on new record bytes, allocation boundaries, finalization
and consecutive native bank/scene commits; do not repeat unrelated passing work.

Connected continuation: recovered real single-peer7E40/7510, fixed16F8 contact
impulse publication/retry, original bank fetch and accumulator consumption.
Source review confirmed order/metadata/gates. Controller peer transport must
complete original writes; host failure is fatal and cannot be replayed. Exact
coincident/self geometry and stage5 capture remain blocked, not guessed.
