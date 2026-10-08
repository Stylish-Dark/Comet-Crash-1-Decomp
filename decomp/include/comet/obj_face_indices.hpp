#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace comet::decomp {

// Native OBJ-source indices; NOT a claim about the recovered PS3 parser's
// unresolved signed-index normalization or source-array ordering.
struct ObjSourceCounts {
    std::size_t positions = 0;
    std::size_t texcoords = 0;
    std::size_t normals = 0;
};

// Zero-based, validated source references. Missing vt/vn uses nullopt.
struct ObjFaceVertex {
    std::size_t position = 0;
    std::optional<std::size_t> texcoord;
    std::optional<std::size_t> normal;

    bool operator==(const ObjFaceVertex&) const = default;
};

enum class ObjReferenceError {
    None,
    Syntax,
    ZeroIndex,
    OutOfRange,
};

struct ObjReferenceResult {
    ObjFaceVertex vertex{};
    ObjReferenceError error = ObjReferenceError::None;
    explicit operator bool() const { return error == ObjReferenceError::None; }
};

// Standard OBJ rules: positive indices are one-based; negative indices refer
// back from the corresponding source-array count at face parse time.
// Rejects 0, overflow, malformed separators, and out-of-range references.
ObjReferenceResult parse_obj_face_vertex(std::string_view token, ObjSourceCounts counts);

// Stable first-appearance deduplication; failure is explicit rather than
// silently truncating the original game's 16-bit index buffer.
struct ObjIndexBuffer {
    std::vector<ObjFaceVertex> unique_vertices;
    std::vector<std::uint16_t> indices;
    bool index_overflow = false;
};

ObjIndexBuffer deduplicate_obj_face_vertices(const std::vector<ObjFaceVertex>& faces);

} // namespace comet::decomp
