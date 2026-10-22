# Original player controller: connected ordinary path

Checkpoint 2026-10-22. Completion-map items P1/P3/P6 advanced; G1 remains unaccepted.

## What now executes

Readable native C++ recovers the ordinary mode0 path from FE5D8 job10290 entry
through113A4 return. Registered craft can now execute original creation,
9A70 preparation, consumed effects,10290 control,6578 commit into both real
entity banks/cells, and projection into NativeConstructionScene's renderer-facing
ArenaState. Previously the caller had to supply controller output independently.
This is an executable integration test, not a graphical gameplay demonstration.
The F3 preview still uses its separate NativePilot policy; no authentic scheduler,
input snapshot or renderer model binding is invented to call that the original loop.

The path includes entry gates, prior position/orientation reload, velocity and
impulse response,7DD0 containment, no-target7E40 result, ordinary1A118 facing,
action/lifecycle gates,113E0 state reset,3238 pending construction and11718 owner
timer/request, plus the original delayed position and quaternion exchanges.
Source forces next-position Y to1 but commits the prior position; directly
committing the new position would remove an original update delay. The consumed
impulse threshold writes C7, not B7.

3238 uses the recovered fixed type7/model33 constructor, preserves unwritten
reused bytes, attempts original2058 allocation, and only clears8000/publishes via
2198 on success. Allocation failure retains8000 and continues. These transports
remain explicit caller-owned boundaries; no allocation algorithm or successful
submission is fabricated.11718 preserves request metadata and emits kind16/model109
only on the original owner-target timer crossing. The existing1E090 publisher can
receive that descriptor.

All recoverable rejection gates precede external callbacks. On NeedsFinalization,
retain advanced work and the returned commit context; resume only finalization,
never10290 again. Both simulation and scene expose that resume API. A blocked3940
still reports NeedsChangeEffects; the wrapper does not claim the missing effects ran.

## Evidence and limits

Private comparison uses the pinned original SPU interpreter as an oracle; it is
not part of the shipped architecture. Original10290,3238/2DF8,7DD0 and1A118 execute;
2058 reservation,2198 submission and1E090 emission are captured caller boundaries.
Seed0x10290,10,000 cases:9,983 accepted ordinary paths match every byte of the
0x680 context, reused256-byte pending body and128-byte owner descriptor, captured
callback payloads and callback counts. Those cases include5,019 reserve attempts,
3,792 publications and251 owner emissions. Eleven cases reach the same original
1E880 contact-effect boundary; six are excluded by the conservative near-Z input
domain. No output-parity claim applies to those six. Original near-axis handling
can reach unsupported numeric behavior; no preview fallback is silently accepted.

Public tests exercise real preparation, transactional rejection, pending allocation,
owner publication, four connected storage/scene updates, delayed visible movement,
wrong-owner rejection and blocked-finalization resumption without repeat callbacks.
A genuinely full one-slot pool supplies allocation failure for scene integration.
This does not establish a working2058/2198 transport implementation.

Release build and72 CTests pass. One unrelated test executable had been pruned
to zero bytes; relinking that target and rerunning only its failed launch passed.
Repository safety and whitespace checks pass. Focused source review identified
and resolved the missing scene finalization-resume wrapper.

This is serial behavior parity for the stated domain, not original compiler byte
matching, hardware concurrency parity, complete controller recovery or whole-game
exactness. Private binaries/assets/disassembly/oracle code are excluded from git.

## Remaining controller and G1 dependencies at PR #65

Updated action/target-facing entries are in [the next checkpoint](original-player-actions.md).

| Work | Original source / concrete missing behavior |
| --- | --- |
| P1 alternate dispatch |11CA8..11DE4,12240/12258 automatic control;10610/119BC and11DE8..12194 attachment state, audio/neighbor effects |
| P2 carry/action |10774..10AE4,11808..119B8,12198..12288,12660..12B80: target selection, pickup, release and connected effects |
| P3 facing/banking/contact |106B0..10750 target facing;10B60..11224,11A80..11CA4 banking; real-peer7E40; energetic1E880 contact effects; near-axis numeric domain |
| P4 lifecycle |12288..1265C,12774..12A70 death, RNG1F030, audioB588, effects1E880 andBB30;3940 finalization change effects |
| P5 transport |2058/2198 pending ownership, queue bootstrap and downstream consumption; request publication alone is insufficient |
| P6 scheduler | Original snapshot/job ownership, frame serial and update order; attach complete controller to running original loop |
| P7/P8 input | Original command translation followed by mouse adaptation; no guessed control snapshot |

Banking reaches2400,180A0 and four temporary entities beyond the existing0x680
work slice; these need owned semantic storage, not an unchecked larger buffer.
Next implement connected action/carry dispatch and its required pending transport,
then remove remaining controller branches and run G1. Do not repeat passing ordinary
path comparisons unless a relevant implementation change requires it.

## Provisional G1 effort forecast

Source inspection supports an initial low/likely/high allowance of **16/32/64 focused
implementation hours remaining for G1**, not the whole port or elapsed calendar time.
This is low confidence and uncalibrated: roughly4/8/16 for remaining controller
branches,8/16/32 for connected carry/contact/lifecycle/queue dependencies, and4/8/16
for original scheduling/input integration and the runnable demonstration. It assumes
existing recovered helpers remain usable and no additional major job architecture
is discovered. These are engineering allowances, not measured throughput or a promise;
revise from completed connected work and newly exposed dependencies. No reliable
full-game completion date follows from them. Source recovery, archive restoration,
private-oracle setup and a clean build all consumed this checkpoint; exact phase
timing was not recorded and is not reconstructed.

Overall full-game planning estimate remains about25%, plausible15–35%. The concrete
movement is P1 ordinary entry-to-commit now connected, with specific P3/P6 gains;
P1 is still Partial and no playable gate has changed to accepted.
