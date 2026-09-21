# DECISIONS

## D001 — Static recompilation is the implementation path

**Decision:** Build the native port around `ps3recomp`, not around shipping an emulator wrapper.

**Reason:** The project goal is a native PC port with an inspectable/modifiable adaptation layer, not simply PS3 execution on PC.

## D002 — Vanilla behavior before enhancements

**Decision:** First reach stable controller-driven gameplay matching the original. Mouse/keyboard, high-refresh work, aspect-ratio changes and other PC polish come afterwards.

**Rejected:** changing input/render timing while core boot/gameplay correctness is still unknown.

## D003 — Preserve the legal boundary

**Decision:** Never commit or redistribute Comet Crash PKG/RAP/RIF files, EBOOT.BIN, decrypted ELF files, extracted copyrighted assets, or Sony SDK material.

**Allowed:** original port code, scripts, non-infringing generated metadata, compatibility reports, address/signature-based patches, documentation.

## D004 — Do not fake gameplay

**Decision:** Missing guest behavior is fixed through faithful lifting/HLE/runtime work. Core gameplay must not be silently replaced with approximate host-side logic just to make the executable run.

## D005 — SPU work is evidence-driven

**Decision:** Do not assume every embedded SPU image must be lifted before first boot. Determine what the title actually dispatches.

**Current evidence:** the larger embedded SPU appears to be Sony MultiStream/MP3 audio middleware. The smaller image remains unresolved.

## D006 — Pin the active ps3recomp revision

**Decision:** Reproducible work currently targets `sp00nznet/ps3recomp@e2815326c58d3530936166982672cb09acdef4f9`.

**Reason:** the 2026-09-21 revision generalizes the lifted-SPU build arrangement into the official project template. Comet Crash's 24 `cellSpurs` imports make that directly relevant. Do not silently move the pin; update this decision when deliberately rebasing.

## D007 — GitHub is the continuity source of truth

**Decision:** Important facts, hypotheses, failures, tooling pins, milestones and next actions are checkpointed here as work proceeds. Chat history is not the project record.
