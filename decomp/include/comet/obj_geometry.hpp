#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace comet::decomp {

// Portable OBJ indexing boundary. This is a native implementation of the OBJ
// standard, NOT yet a claim of instruction-exact PPU 0x00105308 behaviour.
struct ObjSourceCounts {
    std::size_t positions = 0;
    std::size_t texcoords = 0;
    std::size_t normals = 0;
};
struct ObjVertexKey {
    std::size_t position = 0;
    std::optional<std::size_t> texcoord;
    std::optional<std::size_t> normal;
    bool operator==(const ObjVertexKey&) const = default;
};
struct ObjIndexedFaces {
    std::vector<ObjVertexKey> unique_vertices;
    std::vector<std::uint16_t> indices;
};
// Handles p, p/t, p//n, p/t/n; positive 1-based and negative relative indices.
// Zero and out-of-range indices are invalid. Missing position is invalid.
std::optional<ObjVertexKey> parse_obj_vertex_reference(
    std::string_view reference, ObjSourceCounts counts);

// Each polygon has >=3 face tokens. Fan triangulation is a native policy;
// precise legacy polygon/winding semantics still require binary confirmation.
// Transactional: returns nullopt on malformed references or u16 overflow.
std::optional<ObjIndexedFaces> index_obj_polygons(
    const std::vector<std::vector<std::string_view>>& polygons,
    ObjSourceCounts counts);
} // namespace comet::decomp
