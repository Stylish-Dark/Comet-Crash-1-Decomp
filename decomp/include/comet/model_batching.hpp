#pragma once

#include <cstddef>
#include <cstdint>

namespace comet::decomp {

// Proven legacy offsets inside each 0x78-byte submesh record. These fields
// describe the PS3/SPU-prepared batched draw path used by renderer 0x00100750.
// Native backends should translate the semantics rather than preserve the
// original pointer layout.
struct LegacySubmeshBatchOffsets {
    static constexpr std::uint32_t batch_enabled = 0x64;
    static constexpr std::uint32_t batched_instance_count = 0x68;
    static constexpr std::uint32_t batched_vertex_stream = 0x6C;
    static constexpr std::uint32_t batched_index_stream = 0x70;
    static constexpr std::uint32_t batched_object_info_stream = 0x74;
};

inline constexpr std::size_t kLegacyBatchedVertexStride = 0x20;
inline constexpr std::size_t kLegacyBatchedIndexElementSize = 0x04;
inline constexpr std::size_t kLegacyBatchedObjectInfoStride = 0x10;

struct BatchedVertexLayout {
    std::uint32_t stride = 0;

    // The expanded stream binds two vec4 attributes:
    //   +0x00 -> ordinary position-style vertex input
    //   +0x10 -> Cg parameter "normal_ty" / TEXCOORD0
    std::uint32_t primary_vec4_offset = 0;
    std::uint32_t normal_ty_vec4_offset = 0;
};

struct BatchedStreamByteCounts {
    std::size_t vertex_bytes = 0;
    std::size_t index_bytes = 0;
    std::size_t object_info_bytes = 0;
};

constexpr BatchedVertexLayout batched_vertex_layout() {
    return {
        static_cast<std::uint32_t>(kLegacyBatchedVertexStride),
        0x00,
        0x10,
    };
}

constexpr BatchedStreamByteCounts batched_stream_byte_counts(
    std::uint32_t instance_count,
    std::uint32_t model_vertex_count,
    std::uint32_t submesh_index_count) {
    return {
        static_cast<std::size_t>(instance_count) *
            static_cast<std::size_t>(model_vertex_count) *
            kLegacyBatchedVertexStride,
        static_cast<std::size_t>(instance_count) *
            static_cast<std::size_t>(submesh_index_count) *
            kLegacyBatchedIndexElementSize,
        static_cast<std::size_t>(instance_count) *
            kLegacyBatchedObjectInfoStride,
    };
}

// Exact renderer-side count/range expansion for the batched path.
struct BatchedDrawCounts {
    std::uint32_t max_vertex = 0;
    std::uint32_t index_count = 0;
};

constexpr BatchedDrawCounts batched_draw_counts(
    std::uint32_t instance_count,
    std::uint32_t model_vertex_count,
    std::uint32_t submesh_index_count) {
    const std::uint32_t expanded_vertices =
        instance_count * model_vertex_count;
    return {
        expanded_vertices == 0 ? 0 : expanded_vertices - 1,
        instance_count * submesh_index_count,
    };
}

}  // namespace comet::decomp
