# Native OBJ face-index foundation (provisional)

The original loader at PPU `0x00105308` stages 12-byte face-reference
records, deduplicates them into a unique interleaved vertex table, and writes
16-bit indices. Those structural observations are established; the exact
source-index normalization and deduplication equality used by the title are
**not** yet proven from the disassembly.

The new `obj_face_indices` module is an independently testable **standard
OBJ implementation** for the native rewrite, not a claim of binary equivalence:

- `v`, `v/vt`, `v//vn`, `v/vt/vn` face references;
- positive one-based indices and negative relative indices, each checked
  against the relevant source count at the instant the face is parsed;
- explicit zero, malformed, overflow and out-of-range rejection;
- stable first-appearance deduplication using the full position/UV/normal
  source-reference tuple, distinguishing missing fields from present fields;
- explicit failure if more than 65,536 distinct vertices require 16-bit indices.

Do not substitute this for the legacy algorithm in a binary-equivalence test
until the PPU source-index conversion, triangulation, equality comparison and
missing-normal conventions are recovered independently.

CTest target: `comet_obj_face_indices_tests`.

Next reverse-engineering step: inspect the signed arithmetic around the three
source-array lookups in `0x00105308..0x0010B537`, verify whether negative
indices are accepted, and correlate the 12-byte face records with the
deduplication comparison. Record discrepancies before changing this native API.

## Native polygon-to-submesh pipeline

`obj_polygon_mesh.hpp` and `obj_polygon_mesh.cpp` now implement a small
incremental mesh builder using source-triplet equality across submeshes.
It fan-triangulates convex polygons, generates explicit 16-bit triangle
indices, and emits renderer-compatible `SubmeshDrawRange` records with
first index, count and min/max vertices. Every append is transactional:
invalid polygons or vertex/index overflow leave prior mesh data intact.

This is a port-side construction step, **not** newly proven legacy PPU
triangulation semantics. Concave OBJ polygons are not supported by this
fan policy. The dedicated CTest target is `comet_obj_polygon_mesh_tests`.
