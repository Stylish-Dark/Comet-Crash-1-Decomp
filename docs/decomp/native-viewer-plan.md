# Native asset loading and visible graphics milestone

Goal: advance the existing native C++ port from metadata-only bootstrap to rendering the user's original models. Shipping architecture remains the semantic native rewrite in PROJECT_PLAN.md. A viewer is an intermediate milestone, not recovered gameplay.

1. Implement native model containers and transactional OBJ/MTL loading. Reuse validated face parsing, triplet deduplication, draw ranges, geometry scale, signed radius and material properties. Resolve assets beneath the chosen game-data root; keep proprietary data outside git. Verify numeric validation, absent normals, material boundaries, negative indices, scale and failure rollback with executable C++ tests.
2. Load all 37 arena manifest entries from the selected original asset root and report exact missing/invalid files. Establish explicit CPU model ownership. Keep map loading independently selectable.
3. Add optional SDL2/OpenGL native window/render loop, camera orbit/zoom, model cycling, frame-limit/screenshot verification. Render actual model triangles/material colors. Do not fabricate entity placement or missions from still-untyped map records. Verify an actual rendered frame using original assets and headless graphics where available.
4. Recover model parser discrepancies and next gameplay entry points from the original decrypted ELF when available. Record only instruction-supported claims as recovery.
5. Run the C++ suite, Python suite, graphics smoke and repository safety, update STATE/WORK_QUEUE/SESSION_LOG, and push an isolated branch/PR based on PR #31.

Review focus: malformed/nonfinite source values; missing material/texture files; root path escapes; u16 overflow; rendering failure/resource cleanup. Native viewer controls and triangulation are port policies, never evidence of original behavior.
