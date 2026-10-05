#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace comet::decomp {

// The original loader tokenizes OBJ face lines with " /\n\t" as a delimiter
// set. Repeated slashes therefore collapse instead of producing an empty token.
// Its face parser accepts only the three token-count shapes recovered below.
inline constexpr std::size_t kLegacyObjTrianglePositionNormalTokenCount = 6;
inline constexpr std::size_t kLegacyObjTriangleFullTokenCount = 9;
inline constexpr std::size_t kLegacyObjQuadFullTokenCount = 12;
inline constexpr std::int32_t kLegacyObjMissingTexcoord = -1;
inline constexpr float kLegacyObjAttributeEpsilon = 0.0001f;

struct ObjVec2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct ObjVec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// Internal 0x0C staging order recovered from PPU 0x00109A30..0x00109EF4.
// This is deliberately position/normal/texcoord, not OBJ's textual v/vt/vn
// order. A v//vn reference stores texcoord_index == -1.
struct ObjSourceReference {
    std::int32_t position_index = -1;
    std::int32_t normal_index = -1;
    std::int32_t texcoord_index = kLegacyObjMissingTexcoord;
};

struct DecodedObjFace {
    std::array<ObjSourceReference, 6> references{};
    std::uint32_t reference_count = 0;

    constexpr bool supported() const {
        return reference_count != 0;
    }
};

// Exact legacy conversion after decimal parsing: every present OBJ index is
// decremented once. Negative OBJ relative-index semantics are not implemented
// by the original loader.
constexpr std::int32_t normalize_legacy_obj_index(
    const std::int32_t parsed_index) {
    return parsed_index - 1;
}

// component_tokens excludes the leading "f" token and is already split using
// the legacy delimiter behaviour. Supported forms are:
//   6 tokens  -> 3 * (v, vn)       from textual v//vn
//   9 tokens  -> 3 * (v, vt, vn)
//   12 tokens -> 4 * (v, vt, vn), emitted as triangles 0-1-2 and 2-3-0.
// Any other shape was ignored by the original face parser.
DecodedObjFace decode_legacy_obj_face_tokens(
    std::span<const std::int32_t> component_tokens);

struct ObjInterleavedVertex {
    ObjVec3 position{};
    ObjVec3 normal{};
    ObjVec2 texcoord{};
};

struct ObjReferenceCollapseOptions {
    // PPU 0x001085D0 gates normal mismatch splitting with parser state. The
    // shipped NPEB00142 OBJ corpus takes the enabled path; keep the switch
    // explicit for provenance rather than baking the state source into geometry.
    bool split_on_normal_mismatch = true;
};

struct ObjReferenceCollapseResult {
    bool valid = false;
    std::vector<ObjInterleavedVertex> vertices;
    std::vector<std::uint16_t> indices;
};

// Recreates the legacy source-reference collapse used by model loader
// 0x00105308. One base vertex is created per OBJ "v". The first corner that
// references a base position claims its zero normal/UV slots. Later corners
// reuse that base only when the relevant attributes match within 1e-4;
// otherwise a fresh split vertex is appended. Previously appended split
// vertices are intentionally not searched.
ObjReferenceCollapseResult collapse_legacy_obj_references(
    std::span<const ObjVec3> positions,
    std::span<const ObjVec3> normals,
    std::span<const ObjVec2> texcoords,
    std::span<const ObjSourceReference> references,
    ObjReferenceCollapseOptions options = {});

}  // namespace comet::decomp
