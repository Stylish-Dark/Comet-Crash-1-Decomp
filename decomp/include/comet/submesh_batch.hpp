#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comet::decomp {

// Alternate transient batching fields at the tail of each legacy 0x78-byte
// submesh record.  These offsets are provenance only; the native port should
// represent the batch with ordinary host/GPU resources.
struct LegacyExpandedSubmeshOffsets {
    // PPU 0x000FF060 gates batch construction on this byte.
    static constexpr std::uint32_t enabled = 0x64;

    // PPU 0x000FF12C writes the number of active copies grouped into the batch.
    // Renderer 0x001007E8 tests it and 0x001009C8 multiplies it by the base
    // submesh index count.
    static constexpr std::uint32_t instance_count = 0x68;

    // Dynamic stream pointers written at 0x000FF134..0x000FF13C.
    static constexpr std::uint32_t packed_vertex_stream = 0x6C;
    static constexpr std::uint32_t index_stream_u32 = 0x70;
    static constexpr std::uint32_t object_info_stream = 0x74;
};

inline constexpr std::size_t kExpandedBatchVertexStride = 0x20;
inline constexpr std::size_t kExpandedBatchObjectInfoStride = 0x10;
inline constexpr std::size_t kExpandedBatchIndexElementSize = 0x04;

// These names survive in the original PS3 vertex-program parameter table.
inline constexpr std::string_view kExpandedBatchPositionParameter = "position_tx";
inline constexpr std::string_view kExpandedBatchNormalParameter = "normal_ty";
inline constexpr std::string_view kExpandedBatchObjectInfoParameter = "objInfo";

struct ExpandedBatchCounts {
    std::uint32_t instance_count = 0;
    std::uint32_t model_vertex_count = 0;
    std::uint32_t submesh_index_count = 0;

    constexpr std::uint64_t expanded_vertex_count() const {
        return static_cast<std::uint64_t>(instance_count) *
               static_cast<std::uint64_t>(model_vertex_count);
    }

    constexpr std::uint64_t expanded_index_count() const {
        return static_cast<std::uint64_t>(instance_count) *
               static_cast<std::uint64_t>(submesh_index_count);
    }

    constexpr std::uint64_t packed_vertex_bytes() const {
        return expanded_vertex_count() * kExpandedBatchVertexStride;
    }

    constexpr std::uint64_t index_bytes() const {
        return expanded_index_count() * kExpandedBatchIndexElementSize;
    }

    constexpr std::uint64_t object_info_bytes() const {
        return static_cast<std::uint64_t>(instance_count) *
               kExpandedBatchObjectInfoStride;
    }
};

struct ExpandedBatchVertexLayout {
    std::uint32_t stride = static_cast<std::uint32_t>(kExpandedBatchVertexStride);
    std::uint32_t position_tx_offset = 0x00;
    std::uint32_t position_tx_components = 4;
    std::uint32_t normal_ty_offset = 0x10;
    std::uint32_t normal_ty_components = 4;
};

struct ExpandedObjectInfoLayout {
    std::uint32_t stride = static_cast<std::uint32_t>(kExpandedBatchObjectInfoStride);
    std::uint32_t components = 4;
};

// Semantic description of the special renderer path selected when
// submesh+0x68 != 0.  The original implementation allocates/fills transient
// streams (via a 256-byte SPU/SPURS-facing work descriptor) and renders one
// multiplied draw using a u32 index stream.
struct ExpandedSubmeshBatch {
    ExpandedBatchCounts counts{};
    ExpandedBatchVertexLayout vertex_layout{};
    ExpandedObjectInfoLayout object_info_layout{};

    constexpr std::uint64_t draw_index_count() const {
        return counts.expanded_index_count();
    }

    constexpr std::uint64_t draw_max_vertex() const {
        const auto vertices = counts.expanded_vertex_count();
        return vertices == 0 ? 0 : vertices - 1;
    }
};

}  // namespace comet::decomp
