# Controller input reversing

The original game imports six `sys_io` pad functions. The key path is:

- `cellPadGetData` NID `0x8B72CDA1`, import stub `0x001B6D78`.
- Direct caller at `0x000F4CAC`, inside polling function `0x000F4858` (OPD `0x00236FE8`).
- The call receives a `CellPadData` packet at `r1+0x70`.
- After checking the return value and packet length, the game calls decoder `0x000F1500` (OPD `0x00236FC0`).
- Decoder `0x000F1500` reads digital button words at `+0x08/+0x0A` and the four primary analogue bytes at `+0x0C/+0x0E/+0x10/+0x12`, centres the axes around 128 and normalises them into game input state.

## PC-control strategy

1. Compatibility mode: inject mouse/keyboard state at the `cellPadGetData` HLE boundary while leaving controller packets untouched when PC input is inactive.
2. Native pointer mode: trace the state written downstream of `0x000F1500` and drive the actual world/UI cursor directly, avoiding analogue-stick acceleration.
