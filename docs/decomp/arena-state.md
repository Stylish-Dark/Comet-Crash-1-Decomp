# Native arena state and scheduled commands

Evidence: original user-owned EBOOT.ELF SHA-256 `3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.

The native state replaces the supported map-consumption boundaries in `0xD7A48`; it does not reproduce guest memory layout. It retains every supported entity's original record, grid, initial upgrade, variant, position and four orientation words. Full economic and constructor-specific state is not yet recovered.

## Map consumption

The arena uses a fixed 24 by 24 indexing stride. Primary types 10 and 11 set player starts and maximum arena extent. Other supported types call the constructor dispatch at `0xD645C`: 9, 20 through 26, 28 and 29. Their initial model slots are respectively 34, 17, 11, 16, 14, 18, 35, 24, 25 and 21. Positions are grid coordinates plus 0.5, with y=0 except type 9 y=-0.2 (`0xD63E8..0xD6438`, `0xD6CA0`). Terrain's constructor forces owner zero. Source orientation words at record +4 through +16 copy to entity +0x20 through +0x2C.

Primary byte +20 and secondary byte +8 select alternate player routing. Alternate route -1 is skipped before coordinate lookup. Runtime secondary records compress to eight bytes: float time, x, z, selector, opcode. Heap insertion swaps only for strictly earlier timestamps. Native parsing is transactional and adds finite-number/grid/routing checks.

Visibility is 0x9f for terrain, owner+1 in the high nibble for bases, and that nibble OR 15 for other supported structures. Gateway visibility uses unrecovered team state; its native cell explicitly marks visibility unrecovered rather than assigning a guessed team mask.

## Event scheduling and dispatch

`0xF1030` consumes at most one event per call. It requires player +0x23B zero, +0x4C equal 11, and event time strictly less than global clock. Selectors 9 and 10 publish a pending command with entity index -1. Other selectors resolve a target with `0xCA2E0`: selector 0 publishes only if none exists; other selectors publish only if the target type equals the opcode. Failed target conditions reschedule through `0xF0B80`.

Pending fields are entity index, selector, opcode, x, z, source flag. Producer sets source flag 1. Consumer `0xCC9CC` immediately resets selector to idle 11. Selector 0 validates through `0xD6088` with commit disabled and then submits a separate construction ring command. It does not immediately create a building. Entity helpers for selectors 1 through 8 still require recovery. Selectors 9/10 add/subtract 10 from an unnamed global float at +0x2D450C; native code has not assigned an invented gameplay meaning to it.

`0xF0B80` always schedules; incoming r8 is unused. It draws the original random generator, converts to float, multiplies by 2^-30, and computes a delay of u*0.5 for selector 3 or fused u*5+4 for others. Positive extra count schedules cumulative additional delays with selector 1. Full queues still consume RNG draws. Native capacity is explicitly caller configurable, with a provisional default of 4096.

`0x198760` uses a 32-entry shuffled LCG: seed=seed*1664525+1013904223 modulo 2^32. Initialization warms eight steps and fills 32 entries. Each draw advances once, takes table[selector & 31], sets selector to that word, replaces that entry with seed, and returns the low 30 bits. TLS initial seed is 1. Actual game initialization seeds from system time. Native tests fix the seed; there is no guessed replacement generator.

`0xCA2E0` first rejects visibility owner nibbles outside 1..4 or excluded owners. It scans ordered cell entries, skipping category bits 0x0c and 0x10 and excluded entry owners. The native lookup accepts explicit recovered entries and masks. It does not substitute simple cell occupancy. Entry flags originate from entity +0x33 via `0xCA6DC` and `0x111808`; complete construction-stage transitions remain unrecovered.

## Construction validation and debit

`arena_construction` replaces the eligibility boundary in `0xD6088`. It requires no visibility owner unless explicitly overridden, cell entry count !=126, nonzero available entity count (root +0xAB30), no queued duplicate coordinates and sufficient current resource. Supported constructor costs from signed table `0x1BEAA0`: 9=1, 20=20, 21=10, 22=50, 23=25, 24=40, 25=10, 26=60, 28=150, 29=0. Cost override uses 1. Native request and finite-number guards run before lookup.

The queue stores 256 four-byte records and preserves FIFO/wraparound. Its full-queue rejection is a native bounded policy; original producer paths do not check fullness. `commit_arena_construction` invokes a supplied recovered constructor and debits only if it succeeds. Current resource is reduced by cost and capped at maximum (`0x130E98..0x130ED4`). This is only the scalar debit boundary: other `0x130DF0` fields and synchronization are not replaced here. Full constructor state and original construction-ring consumption remain to be recovered and wired into the simulation. Validation, submission and commit are separate operations.

## Environment and rendering

`0xEB170` constructs themed slots 34 (obstacle), 37 (comet) and 38 (border). Themes 0..3 use ice, rock, gaseous and lava assets. Obstacle geometry scales are 1, 1.8, .95, .95; signed radius scale .85, options 0x47. Comet scale 1/options 0x4f; border scale 1/options 0x414. Theme 4 is explicitly unsupported for native environment graphics.

Background position is (extentX-12,-.2,extentZ-12) when theme/level meets thresholds (0/3, 1/10, 2/17, 3/24), otherwise (12,-.2,12). Global subroot is proved root+0xA00 by constructor `0xD4564..0xD51A8`; its +0x28D8/+0x28DC are arena extents. Background quaternion is (0,sin(theta/2),0,cos(theta/2)), theta=((level % 4)+1)*pi/2 (`0xEB538..0xEB680`). Native libm rounding may differ from original sin/cos.

Border position is (extentX/2,-.19,extentZ/2), identity orientation. It exists only for bootstrap mode 3; the normal native initial-state preview does not assume this mode. Multiplayer/arena mode names remain unproved.

`--arena` renders recovered map entities and their themed background. Display quaternion conversion, camera, lighting, orbit controls and diffuse shader are native viewing policies. Composite weapon/base parts, original effects, HUD, input/game loop, movement, combat and progression remain incomplete. This is an initial-state arena preview, not a finished playable port.

## Validation

Tests cover map routing and transactionality, strict event timing and one-event consumption, pending/retry target conditions, blocked/idle gates, equal-time heap behavior, exclusion masks, shuffled RNG reference vectors, seed reset, full-queue RNG consumption, delay branches and cumulative repeats. Graphics tests render arena instances and reject missing model slots. Original assets and binaries are never committed.
