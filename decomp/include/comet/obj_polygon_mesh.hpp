#pragma once

#include "comet/model_geometry.hpp"
#include "comet/obj_face_indices.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace comet::decomp {

// Native OBJ polygon builder. Fan triangulation is a chosen port policy,
// not yet established as the original PPU loader's exact face ordering.
// Polygons must be convex, ordered and planar for fan triangulation to be
// geometrically faithful; concave polygons require a separate triangulator.
enum class ObjPolygonError {
    None,
    TooFewVertices,
    IndexOverflow,
    IndexCountOverflow,
};

struct ObjPolygonResult {
    ObjPolygonError error = ObjPolygonError::None;
    explicit operator bool() const { return error == ObjPolygonError::None; }
};

class ObjTriangleMeshBuilder {
public:
    // Build local 16-bit index buffers with first-appearance vertex identity.
    // On failure, neither mesh vertices, indices nor ranges are modified.
    ObjPolygonResult append_submesh(std::span<const std::vector<ObjFaceVertex>> polygons);

    const std::vector<ObjFaceVertex>& vertices() const { return vertices_; }
    const std::vector<std::uint16_t>& indices() const { return indices_; }
    const std::vector<SubmeshDrawRange>& ranges() const { return ranges_; }

private:
    std::vector<ObjFaceVertex> vertices_;
    std::vector<std::uint16_t> indices_;
    std::vector<SubmeshDrawRange> ranges_;
};

} // namespace comet::decomp
