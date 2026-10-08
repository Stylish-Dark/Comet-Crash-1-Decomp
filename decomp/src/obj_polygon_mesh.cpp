#include "comet/obj_polygon_mesh.hpp"

#include <algorithm>
#include <functional>
#include <limits>
#include <unordered_map>
#include <utility>

namespace comet::decomp {
namespace {
// Distinguish missing UV/normal references from present index zero.
struct SourceVertexHash {
    std::size_t operator()(const ObjFaceVertex& v) const noexcept {
        std::size_t h = std::hash<std::size_t>{}(v.position);
        const auto mix = [&h](std::size_t x) {
            h ^= x + std::size_t{0x9e3779b9} + (h << 6) + (h >> 2);
        };
        mix(v.texcoord.has_value());
        if (v.texcoord) mix(*v.texcoord);
        mix(v.normal.has_value());
        if (v.normal) mix(*v.normal);
        return h;
    }
};
} // namespace

ObjPolygonResult ObjTriangleMeshBuilder::append_submesh(
    std::span<const std::vector<ObjFaceVertex>> polygons) {
    std::size_t added_indices = 0;
    for (const auto& polygon : polygons) {
        if (polygon.size() < 3) return {ObjPolygonError::TooFewVertices};
        const auto triangles = polygon.size() - 2;
        if (triangles > (std::numeric_limits<std::uint32_t>::max() -
                          added_indices) / 3)
            return {ObjPolygonError::IndexCountOverflow};
        added_indices += triangles * 3;
    }
    if (indices_.size() > std::numeric_limits<std::uint32_t>::max() ||
        added_indices > std::numeric_limits<std::uint32_t>::max() - indices_.size())
        return {ObjPolygonError::IndexCountOverflow};

    // Stage mutations until the entire submesh has passed validation.
    // An unordered map avoids the original quadratic vertex lookup.
    auto staged_vertices = vertices_;
    std::unordered_map<ObjFaceVertex, std::uint16_t, SourceVertexHash> lookup;
    lookup.reserve(staged_vertices.size());
    for (std::size_t i = 0; i < staged_vertices.size(); ++i)
        lookup.emplace(staged_vertices[i], static_cast<std::uint16_t>(i));

    std::vector<std::uint16_t> staged_indices;
    staged_indices.reserve(added_indices);
    for (const auto& polygon : polygons) {
        std::vector<std::uint16_t> face;
        face.reserve(polygon.size());
        for (const auto& ref : polygon) {
            const auto it = lookup.find(ref);
            if (it != lookup.end()) {
                face.push_back(it->second);
                continue;
            }
            if (staged_vertices.size() >= 65536)
                return {ObjPolygonError::IndexOverflow};
            const auto index = static_cast<std::uint16_t>(staged_vertices.size());
            staged_vertices.push_back(ref);
            lookup.emplace(ref, index);
            face.push_back(index);
        }
        for (std::size_t i = 1; i + 1 < face.size(); ++i) {
            staged_indices.push_back(face[0]);
            staged_indices.push_back(face[i]);
            staged_indices.push_back(face[i + 1]);
        }
    }

    SubmeshDrawRange range{};
    range.first_index = static_cast<std::uint32_t>(indices_.size());
    range.index_count = static_cast<std::uint32_t>(staged_indices.size());
    if (!staged_indices.empty()) {
        const auto [lo, hi] = std::minmax_element(
            staged_indices.begin(), staged_indices.end());
        range.min_vertex = *lo;
        range.max_vertex = *hi;
    }

    // Preserve the strong guarantee on ordinary allocation failures too:
    // construct all new vectors before committing the three swaps.
    auto new_indices = indices_;
    auto new_ranges = ranges_;
    new_indices.insert(new_indices.end(), staged_indices.begin(), staged_indices.end());
    new_ranges.push_back(range);
    vertices_.swap(staged_vertices);
    indices_.swap(new_indices);
    ranges_.swap(new_ranges);
    return {};
}

} // namespace comet::decomp
