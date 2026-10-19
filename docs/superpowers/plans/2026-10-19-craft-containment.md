# Craft containment implementation plan

> **For agentic workers:** Use superpowers:executing-plans to implement inline.

**Goal:** Replace NativePilot's X/Z clamp with the recovered craft world-boundary call in original update order.

**Architecture:** Keep the free-flight API and its numeric fixtures unchanged. Add a bounded composition API in the same arithmetic unit: integrate old velocity, call recovered 7DD0/7A50 six-plane containment, then respond toward the desired velocity using the contact-adjusted velocity. Commit state only after every stage succeeds.

**Tech Stack:** C++20, CMake/CTest, SDL2, Python private instruction oracle.

**Spec:** docs/decomp/native-ground-contact.md and docs/decomp/native-craft-flight.md; new evidence: 10548 calls 7DD0 with integer arena dimensions, floor 0, ceiling 2, no optional sphere points.

## Global constraints

Preserve round-toward-zero and caller rounding restoration. Do not commit original bytes/private oracle inputs. NativePilot's primary/lateral radius .5, restitution 100 and movable flag are explicit preview policy, not recovered craft initialization. Keep unrelated script changes untouched. Publish above PR #55; do not merge.

## Review focus

- Contact must precede velocity response; test outward wall velocity with neutral commands.
- Invalid bounds/overflow must leave input state unchanged.
- Floor and ceiling must use distinct primary/lateral radii and the original flag.
- Repeated diagonal movement must remain in the arena and allow escape from walls.
- Existing pause/reset/gamepad behavior must remain usable.

## Task 1: Recover and compose containment

- [x] Write numeric original-instruction fixtures and tests for no contact, each plane, corners, immovable/separating velocity, radius flag and both response branches. Run the new behavior assertion against old flight to establish failure.
- [x] Add ArenaCraftBounds and advance_arena_craft_in_arena to arena_craft_flight.hpp/.cpp, returning motion-only contact metadata. Compare 10,000 deterministic cases with untouched original flight blocks and 7DD0, suppressing only external effect callbacks.
- [x] Replace NativePilot clamp with the composed API; verify inward movement after wall contact and update its bounds tests.
- [x] Run complete CPU/graphics CTests, Python regressions, focused ASan/UBSan, and independent review. Document actual evidence, limits and capability estimate; commit and publish a stacked PR.


Execution record: new pilot wall assertion failed against old clamp, then passed
with containment. 10,000 original-instruction queries match exactly (details in
native-craft-containment.md). 63 CPU/233 Python/four focused sanitizers pass.
Independent review found no actionable findings. Initial graphics keyboard test
failed its existing fixed-delay construction assertion; unchanged focused rerun
passed. Final full graphics rerun and publication recorded in the milestone doc.
No additional design rulings or deferred review findings.

Final complete graphics rerun:66/66 passed in32.37s. Publication proceeds as a
reviewable branch/PR stacked above #55 under the existing autonomous mandate.
