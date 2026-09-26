# PROJECT PLAN

## End state

A normal Windows-native Comet Crash executable built from recovered game source, not from a PowerPC register machine plus a PS3 compatibility runtime.

The finished port should:
- preserve original gameplay, missions, assets and controller behaviour;
- express game logic in readable, maintainable C/C++;
- use native PC graphics, audio, input, filesystem and threading APIs;
- support practical mouse/keyboard and modern display behaviour after vanilla parity is established;
- require users to supply their own legally obtained game data where proprietary assets remain necessary;
- contain no Sony SDK material or redistributed proprietary executable data.

## Architecture

1. **Recovered game-domain source**
   - Semantic C/C++ reconstructed from the NPEB00142 v1.00 executable.
   - Stable names/types added only when supported by binary evidence.
   - Raw PPU addresses retained as provenance until each function is confidently identified.

2. **Native platform layer**
   - Rendering backend suitable for PC.
   - Native audio, input, windowing, filesystem, timing and threading.
   - No `cell*`, GCM/RSX, SPURS or PS3 ABI semantics above this boundary.

3. **Asset/data compatibility**
   - Load original game data formats directly where practical.
   - Document and reimplement parsers rather than embedding original executable code.

4. **Reverse-engineering oracle**
   - Exact PS3 ELF analysis, disassembly and `ps3recomp` lift remain available for control-flow verification.
   - Existing runtime traces are used for differential behavioural testing.
   - Static recompilation is a tool, not the shipping architecture.

5. **Continuity and validation**
   - Every recovered function is address-linked to its source evidence.
   - Behavioural tests compare native recovered code to known original outputs/states where possible.
   - Repository state, decisions and next work remain documented for cross-session continuation.

## Phases

### Phase A — Recovery infrastructure
Status: **in progress**.

- Build address/function/string/call-graph inventories from the exact ELF.
- Establish recovered-source conventions and provenance.
- Identify high-value game-domain clusters rather than SDK/library code.

### Phase B — Core game-state and data structures
Status: **started**.

- Recover the large root game/arena state and its major subobjects.
- Recover level-map loading and map format semantics.
- Recover entity/structure registries and asset bootstrap.

### Phase C — Gameplay simulation
Status: **not started**.

- Recover update loop, entities, towers/weapons, resources, enemies, waves and mission state.
- Build deterministic tests around recovered logic before rendering integration.

### Phase D — Native rendering and UI
Status: **not started**.

- Recover `arenaGraphics` and renderer-facing game structures.
- Replace PSGL/GCM/RSX calls with a native renderer.
- Recover fonts, HUD, menus, post-processing and scene composition.

### Phase E — Native audio/input/persistence
Status: **not started**.

- Replace MultiStream/PS3 audio with native playback while preserving game-trigger semantics.
- Recover controller abstraction, then add first-class keyboard/mouse.
- Reimplement save/settings data paths.

### Phase F — Full-game parity and PC polish
Status: **not started**.

- Campaign/mission parity, local multiplayer, save/load and clean shutdown.
- Resolution/aspect/high-refresh work after simulation timing is understood.
- Packaging and user-facing configuration.

## Engineering constraints

- Do not call register-level lifted code "decompiled source".
- Do not invent semantic names to make progress look cleaner than it is.
- Prefer one fully understood subsystem over thousands of mechanically translated functions.
- Preserve old static-recomp work as evidence/oracle until the corresponding native subsystem replaces it.
