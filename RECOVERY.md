# Recovery record

On 2026-09-22, the user supplied a checkpoint ZIP containing a clean Git repository for this exact project.

Original recovered history (oldest → newest):

```text
77f918c feat: rebuild Comet Crash port scaffold with SPU pipeline
b4744c3 feat: restore PS3 import and callsite probing
92f86c5 fix: parse compact PS3 PPU OPD descriptors
cdd5939 feat: decrypt Comet Crash FREE-NPDRM SELF locally
cac15a4 docs: record upstream analysis checkpoint
89e35bf fix: gate lifted PPU and SPU compatibility
53ee1da feat: add Windows mouse and in-game host settings
ca4fd3d feat: automate Windows build and host controls
aaa00a6 Handle Comet SPURS urgent jobchain commands
76bccc8 Handle Comet optional-service HLE gaps
0f217a5 Gate Comet builds on complete HLE coverage
78bbecd Fix Comet HDD boot and VFS mappings
944a29b Fix RESC guest interlace table writes
569568e Add Comet GCM report HLE support
3250838 Fix Comet RSX report completion paths
```

The recovered tree was clean at `3250838` and passed 57/57 unittest tests. It was then continued in the current session with durable first-boot diagnostics and continuity files; that work passed 61/61 tests and was committed locally as:

`c0089d49167172760a2852ec6909dfed58cb2421` — **Add durable first-boot diagnostics and continuity**

Persistent recovery artifacts:

- `/Projects/Comet Crash/Comet-Crash-1-Decomp-checkpoint-with-git.zip` — original checkpoint including `.git` history.
- `/Projects/Comet Crash/Comet-Crash-1-Decomp-recovered-c0089d4.zip` — clean source archive at `c0089d4`.

Original uploaded checkpoint SHA-256: `873f0a8e94828c4d5c6189eb35442313d9e7cf858784586d784b46763ce49cae`.

Clean recovered source ZIP SHA-256: `e78a3e659b3e3fc34a5f2743c9a7181d40cdf73d7b9e14ce7b03af8b50dbf887`.

The present GitHub working tree was created from an earlier temporary reconstruction and may still contain stale scaffold files until the recovered tree is fully replayed. Do not treat those stale files as stronger evidence than `CONTINUITY.md` or the preserved recovery artifacts.
