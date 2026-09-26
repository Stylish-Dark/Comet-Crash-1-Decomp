#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace comet::decomp {

// Top-level legacy model layout proven jointly by the OBJ loader at 0x00105308
// and renderer at 0x00100750. These offsets are provenance only; the native
// rewrite is not required to preserve the PS3 object ABI.
struct LegacyModelGeometryOffsets {
    static constexpr std::uint32_t options = 0x00;
    static constexpr std::uint32_t vertex_count = 0x04;
    static constexpr std::uint32_t submesh_count = 0x08;
    static constexpr std::uint32_t index_count = 0x0C;
    static constexpr std::uint32_t full_vertices = 0x10;
    static constexpr std::uint32_t compact_vertices = 0x14;
    static constexpr std::uint32_t submeshes = 0x18;
    static constexpr std::uint32_t indices = 0x1C;
    static constexpr std::uint32_t full_vertex_buffer = 0x20;
    static constexpr std::uint32_t compact_vertex_buffer = 0x24;
};

inline constexpr std::size_t kLegacyFullVertexStride = 0x20;
inline constexpr std::size_t kLegacyCompactVertexStride = 0x14;
inline constexpr std::size_t kLegacySubmeshStride = 0x78;
inline constexpr std::size_t kLegacyObjFaceReferenceStride = 0x0C;
inline constexpr std::size_t kLegacyIndexElementSize = 0x02;
inline constexpr std::size_t kLegacyMaterialSubobjectOffset = 0x10;

enum class VertexSemantic {
    Position,
    Normal,
    TexCoord0,
};

struct VertexAttributeSpec {
    VertexSemantic semantic{};
    std::uint32_t byte_offset = 0;
    std::uint32_t component_count = 0;
};

struct RecoveredVertexLayout {
    std::uint32_t stride = 0;
    std::array<VertexAttributeSpec, 3> attributes{};
    std::uint32_t attribute_count = 0;
};

// 32-byte path: position.xyz, normal.xyz, texcoord.xy.
RecoveredVertexLayout full_vertex_layout();

// 20-byte path: position.xyz, texcoord.xy. The renderer selects this path when
// model+0x10 is null and binds model+0x24 instead of model+0x20.
RecoveredVertexLayout compact_vertex_layout();

}  // namespace comet::decomp
