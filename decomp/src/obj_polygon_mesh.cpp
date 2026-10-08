#include "comet/obj_polygon_mesh.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace comet::decomp {

ObjPolygonResult ObjTriangleMeshBuilder::append_submesh(
    std::span<const std::vector<ObjFaceVertex>> polygons) {
    for (const auto& polygon : polygons)
        if (polygon.size() < 3) return {ObjPolygonError::TooFewVertices};

    // Commit only after all count/index validations pass.
    auto new_vertices = vertices_;
    auto new_indices = indices_;
    auto new_ranges = ranges_;
    std::vector<ObjFaceVertex> local;
    std::vector<std::uint32_t> triangle_indices;

    // Use equality over full source triplets, matching the native dedup policy.
    // A linear lookup is deliberately simple here; most OBJ faces have 3-4
    // vertices, and deduplication across submeshes is handled by this builder.
    for (const auto& polygon : polygons) {
        std::vector<std::uint32_t> face_indices;
        face_indices.reserve(polygon.size());
        for (const auto& ref : polygon) {
            auto pos = std::find(new_vertices.begin(), new_vertices.end(), ref);
            if (pos == new_vertices.end()) {
                if (new_vertices.size() >= 65536)
                    return {ObjPolygonError::IndexOverflow};
                face_indices.push_back(static_cast<std::uint32_t>(new_vertices.size()));
                new_vertices.push_back(ref);
            } else {
                face_indices.push_back(static_cast<std::uint32_t>(
                    std::distance(new_vertices.begin(), pos)));
            }
        }
        for (std::size_t i = 1; i + 1 < face_indices.size(); ++i) {
            triangle_indices.push_back(face_indices[0]);
            triangle_indices.push_back(face_indices[i]);
            triangle_indices.push_back(face_indices[i + 1]);
        }
    }

    if (new_indices.size() > std::numeric_limits<std::uint32_t>::max() ||
        triangle_indices.size() >
            std::numeric_limits<std::uint32_t>::max() - new_indices.size())
        return {ObjPolygonError::IndexCountOverflow};

    SubmeshDrawRange range{};
    range.first_index = static_cast<std::uint32_t>(new_indices.size());
    range.index_count = static_cast<std::uint32_t>(triangle_indices.size());
    if (!triangle_indices.empty()) {
        const auto [lo, hi] = std::minmax_element(triangle_indices.begin(),
                                                  triangle_indices.end());
        range.min_vertex = *lo;
        range.max_vertex = *hi;
    }
    new_indices.reserve(new_indices.size() + triangle_indices.size());
    for (const auto index : triangle_indices)
        new_indices.push_back(static_cast<std::uint16_t>(index));
    new_ranges.push_back(range);
    vertices_ = std::move(new_vertices);
    indices_ = std::move(new_indices);
    ranges_ = std::move(new_ranges);
    return {};
}

} // namespace comet::decomp
