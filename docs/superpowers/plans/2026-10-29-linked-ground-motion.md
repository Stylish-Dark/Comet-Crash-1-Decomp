# Linked ground motion implementation plan

**Goal:** Remove the supplied14060-motion boundary from the linked ground path and recover failed pickup timeout.
**Architecture:** Extend existing carry recovery with14090..14420's stage5, valid-target, linked branch after completed9A70. Reuse formation, world contact and13998 steering; preserve source scratch writes and explicit unsupported peer/contact branches. Connect a simulation/scene method through motion,158B8 and finalization. No replacement simulation rules.
**Contract:** AGENTS.md P2/P3/P6; original FE5D8 source and existing original-ground-carry.md. Full ordinary14060, target reacquisition and unknown contact/capture branches remain named boundaries.

- [x] Recover motion order, peer gates, world bounds, cell/suppression state and steering directly from source; compare complete context at14090 entry against original instructions.
- [x] Connect actual registered unit and parent through recovered motion/dynamics/bank commit/scene projection; ensure rejection preserves state.
- [x] Recover16008..16060/171F0 timeout, preserve command2/3 bypass and bit21-only gate; compare source outputs and exercise a failed pickup followed by restored capacity.
- [x] Focused build and relevant scenarios, source review, record precise remaining boundaries and publish checkpoint without merging stack.

Review focus: real parent identity; packed W lanes; peer effects must not be skipped; target validity; command acknowledgement must bypass timeout; external publication cannot be replayed on finalization refusal.

Execution record: added source12BE0/3068/2CE0 target reacquisition to close the
linked caller's lost-target branch, rather than leaving it as an external input.
Independent review corrected BA(any contact) and post-effect failure phase.
Focused evidence is recorded in original-linked-ground-motion.md. Existing .cmd
line-ending changes are unrelated and excluded from publication. No stack merge.
