# Native graphics and original asset loading

The `comet_native` executable now has an optional SDL2/OpenGL viewer independent
of the old PS3 compatibility host. It loads the original OBJ/MTL files, uploads
32-byte native vertices and 16-bit indices, decodes diffuse DDS images, and draws
original submeshes with a native orbit camera. This is a visible native port
milestone, **not a playable game**.

## Build

Default headless recovery build requires only a C++20 compiler and CMake:

```sh
cmake -S decomp -B build-decomp
cmake --build build-decomp
ctest --test-dir build-decomp --output-on-failure
```

Graphics build additionally requires SDL2 development files and OpenGL:

```sh
cmake -S decomp -B build-native -DCOMET_NATIVE_VIEWER=ON
cmake --build build-native
ctest --test-dir build-native --output-on-failure
```

The new `native-source-port` workflow builds this exact target on Windows and
Linux. The Windows artifact includes the executable, SDL2 DLL and drag-folder
launcher. Windows runs the CPU/CLI tests; Linux also runs the offscreen rendered
pixel test. Graphics tests have the `graphics` label, permitting `ctest -LE graphics`
on hosts without a graphics context. Existing assert-based recovery checks stay
active even in Release builds.

## Original data

Use the directory containing `models`, typically `<title>/USRDIR/data`. Game
files remain local and outside git. Built-in maps can be extracted with the
existing `tools/extract_builtin_level_maps.py` after decrypting the user's EBOOT.

```sh
./build-native/comet_native --assets /path/to/USRDIR/data --validate-assets
./build-native/comet_native --assets /path/to/USRDIR/data --viewer
./build-native/comet_native --assets /path/to/USRDIR/data --object models/care/cometRock_03/cometRock.obj --viewer
./build-native/comet_native /path/to/USRDIR/data 0 --assets /path/to/USRDIR/data --viewer
```

Arrows cycle model entries; left-drag orbits; wheel zooms; W toggles wireframe;
R resets the camera; Escape closes. `--frames N --hidden --screenshot frame.ppm`
provides bounded graphical verification without claiming gameplay.

## Implemented boundaries

- Transactional OBJ model assembly, source-triplet deduplication and indexed submesh ranges.
- Geometry scale, optional Y offset, signed radius and native geometry bounds.
- MTL colors/specular exponent/team policy and diffuse/specular/bump/cube filenames.
- Confined asset-root resolution, including rejection of escaping relative paths and symlinks.
- Shipped `.tif` exporter texture names resolved to `.dds`; stale absolute author paths use their basename under the MTL directory. This basename adaptation is native policy.
- DDS top-mip decoding: masked RGB/luminance and DXT1/DXT3/DXT5; cube/volume formats reject explicitly.
- GPU vertex/index buffers and diffuse textures; recoverable errors and RAII cleanup before destroying the graphics context.
- All 11 recovered arena target descriptors materialized as textures and all 10 framebuffer configurations checked complete. The display-scaled primary color/depth target receives the model draw and is blitted to the window. Remaining recovered targets are allocated for later scene/postprocess integration.

## Real-title verification in this session

The locally decrypted NPEB00142 v1.00 ELF matched SHA-256
`3b4b6fef525ac0893fd96f7f53d84affd8c9d2586a71a45341a76e8ba78497c6`.
All 37 arena manifest entries loaded: **20,802 native vertices, 7,506 triangles**.
The static rock-comet model loaded: **625 vertices, 1,152 triangles**.
The viewer rendered bounded actual-title frames under Mesa llvmpipe; level 0
bootstrap independently reported extent 16, 147 primary records and 1 secondary record.
No original assets, ELF or instruction dump are committed.

## Remaining work

Original material shaders (normal/specular/team/emission), arena scene placement,
map entity interpretation, update/simulation, missions, HUD, audio, input parity,
save/load and multiplayer are not implemented. Standard OBJ signed-negative-index
handling, generated missing normals, convex-fan polygon construction and viewer
controls are native policies until independently verified. A successful graphics
frame is not evidence of full gameplay parity.
