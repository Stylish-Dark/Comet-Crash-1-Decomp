# Native arena bootstrap executable

The new `comet_native` target is a real, ordinary C++20 executable linked
against the recovered native source library, independent of the old
static-recomp host. The default build is a headless arena/bootstrap harness; an optional native
SDL2/OpenGL viewer now renders original models and DDS textures. **Not a playable source port yet.**

Build from the repository:

```sh
cmake -S decomp -B build-decomp
cmake --build build-decomp
ctest --test-dir build-decomp --output-on-failure
```

Use level maps extracted locally from a legitimate NPEB00142 v1.00 copy
(`tools/extract_builtin_level_maps.py`):

```sh
./build-decomp/comet_native /path/to/extracted/maps 0
```

The executable currently loads `level0.map`, verifies and normalizes its
recovered disk records, constructs the recovered render-target plan and
arena model manifest as native-owned application data, then reports
their counts. It does **not** yet open a graphics window, upload meshes,
simulate entities or execute missions.

`NativeSession` establishes an explicit native state boundary and
transactional level changes: failure to load the destination preserves
the current loaded level. Tests create a synthetic map and verify
bootstrap, manifest population, extent normalization, and rollback.

## Next vertical slice

1. Implement an asset filesystem abstraction and resolve the 37
   model manifest paths from a selected game-data root.
2. Load model geometry through the native OBJ parser and create
   CPU-side `Model`/`Submesh` containers.
3. Add an SDL or platform-window-backed renderer and materialize
   `ArenaRenderTargetPlan` into real graphics resources.
4. Establish a game update loop and draw the first static arena
   from recovered map records before implementing towers and waves.

No recovered gameplay behaviour is claimed by this executable yet.

## Native graphics milestone (2026-10-09)

See `native-viewer.md` for graphics builds, original data-root selection and controls.
CPU-side original model loading, GPU vertex/index buffers, diffuse DDS textures,
and the recovered 11-texture/10-framebuffer plan are now implemented. The first
arena geometry can be inspected with `--object models/care/cometRock_03/cometRock.obj`.
Map records are still not instantiated as entities. Full simulation remains the next major boundary.
