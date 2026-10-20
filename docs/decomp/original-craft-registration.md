# Original player-craft spawn and registration

## Scope

The recovered source can now create a type-1 craft in the real entity storage
and connect it to its player, rather than only preparing a temporary collision
profile. `ArenaConstructionSimulation::create_player_craft` connects recovered
position selection, body initialization, FIFO insertion, both banks/cell lists,
player/root links and the original ordered auxiliary requests.

This API is explicit: the existing F3 preview does not automatically invoke it.
No original carrying, combat or full craft update is claimed by creation alone.

## Source evidence

Pinned NPEB00142 v1.00 ELF SHA-256:
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
The original instructions were decoded from that private ELF with Capstone;
no original executable bytes or disassembly are distributed here.

| Original location | Recovered contract |
| --- | --- |
| CF2A0..CF2BC | Selected configured X >= 0 uses configured X/Z |
| CFFC8..D0020 | Missing X uses arena halves and owner division/remainder to place four owners around the center |
| CEFC4 | f22 is .5 in this spawn-selection path |
| CFA54..CFA78 | Truncate X/Z toward zero; call1326C0(category6, X, Z, flag1, scalar0), ignore result |
| CFA80..CFB88 | Prepare X/zeroY/Z and preserve staged W; existing original type-1 constructor |
| CFB8C -> CA514 | Allocate FIFO identity, copy the 256-byte body into both banks, insert both ordered cell records, increment summary |
| CFBC8 | Zero-extended body index -> player+220 |
| CFBD8 | Same index -> root+3224+2*owner |
| CFBDC/CFBE8 | Second category6 request, same X/Z/flag/scalar; store returned word at player+228 |
| CFC00/CFC0C | Category5 request, same other arguments; store returned word at player+22C |

The owner stride of two for the root registry follows the loop cursor increment
at CF5AC/CF5B8. `ArenaPlayerCraftRegistry` stores the same identities as native
optional indices; `ArenaPlayer` holds the corresponding body link and two signed
auxiliary words. Existing C++ value copies preserve those links with the banks.

The fallback is:

```
x = float(width / 2 + owner % 2 - 1) + .5
z = float(depth / 2 + owner / 2 - 1) + .5
y = 0
```

Extent halves are unsigned integer division; owner division/remainder is signed
in the original, equivalent for native owners0..3. Staged W is preserved in the
constructor's +EC. Configured X==0 is accepted; a negative X selects fallback
regardless of the configured Z. This is not a spawn at a native base cursor.

## Audio dependency, not fabricated completion

The three calls are the same original1326C0 boundary. Its downstream path creates
voices and configures 44,100 Hz data at132FA4..132FDC; this identifies an audio
subsystem dependency. Category5/6 names and the scalar's gameplay meaning are
not assigned. The boundary is represented by a required callback receiving all
five arguments. No callback means rejection, not an implicit successful stub.
A supplied callback may return -1 as the original can; creation still finishes
and retains those return words. The first call's result is intentionally ignored.

The first request precedes body creation; the next sees both body links; +228 is
written before the last request. No transaction wrapper rolls back these source
side effects or delays their visibility. Callback implementations must not throw,
mutate the storage or re-enter creation. Actual native audio recovery remains open.

## Native admission and remaining boundaries

The API validates native owners0..3, extents1..24, finite bounded coordinates,
zero spawn Y, an unlinked player/root entry and available bank/pool/cell capacity
before any source side effect. Out-of-arena resolved positions are rejected.
These are explicit native admission rules, not newly recovered original failure
branches. The original successful bootstrap dereferences the registration result;
this API does not invent behavior for that original null-result path.

The caller supplies reused scratch-slot bytes. Unwritten bytes remain intact;
no guessed zero initialization is claimed. The simulation method stores original
links on the actual player and registry while preserving resource accounts,
production quotas and unrelated owners. The existing NativePilot remains a
separate preview object until the complete original update contract is recovered.

Still open: root/mode choice of the configured position pair, the surrounding
arena bootstrap admission branches, complete9A70 preparation/10290 controller/
6578 commit and original scheduling, action/carry/release, actual audio playback,
and scene/input ownership integration. Aircraft, AI, missions and campaign remain
separate substantial gaps. Neither byte-matching executables nor whole-controller
instruction parity is claimed for this milestone.

## Validation

The new public C++ test first failed on missing registration, then passed after
implementation. The center-fallback integration assertion separately failed on
the explicit-position-only adapter before source fallback recovery. Tests cover
real simulation banks, ordered packed records, FIFO IDs, scratch preservation,
owner/root links, handle visibility during callbacks, negative auxiliary returns,
configured X==0, four-owner rectangular fallback, checkpoints, duplicate/invalid/
missing-dependency/capacity admission and unchanged economic accounting.

The existing constructor and CA514 arithmetic are reused; no repeated large
numeric oracle campaigns were run for unchanged leaves. This milestone adds
source-traced composition and integration checks, not a new hardware-parity claim.

Validation: the Release native build completed without warnings. All66 headless
checks passed:64 in the full run, then the two tests that could not launch until
local executable permissions were restored passed on rerun. No source failure
was masked. Focused source review found no material issue and ran no duplicate
suites. Local graphics/Windows/audio playback were not exercised; CI is separate.
