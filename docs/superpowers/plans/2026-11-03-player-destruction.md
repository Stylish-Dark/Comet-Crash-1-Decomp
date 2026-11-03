# Original player destruction/reset implementation plan

Goal: close10290's12288 destruction boundary through original effects, statistics,
base-enabled reset or permanent removal, continuing through banks and scene.
Contract: AGENTS.md P1/P4/P5/P6; source FE5D8 12288..1265C/12774..12A70.

- [x] Recover pure destruction state/configuration and shared BF58/BB30 reservation updates; compare complete original context, event ordering, RNG and statistics within explicit native transport boundaries.
- [x] Integrate with10290 deferred effects and real simulation-owned statistics, including reset continuation and no-base tombstone finalization. Preserve missing-callback transactional rejection and finalization resumption.
- [x] Exercise actual registered craft damage→reset→movement, permanent death and proxy cleanup, reservation retry and pool exhaustion; independent source review, focused builds and publish without merging the stack.

Keep unknown ship pickup, original scheduling/bootstrap and campaign explicit.
No invented respawn policy, sounds, particle parameters, penalties or timers.

Completed scope/evidence: docs/decomp/original-player-destruction.md.
