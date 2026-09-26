#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace comet::decomp {

// Model objects constructed by the original arena bootstrap occupy a regular
// 0x90-byte table beginning at root-state offset 0x2D2DC0.  The native port
// does not preserve that ABI layout; slot/offset values are provenance only.
struct ArenaModelAssetSpec {
    std::string_view path;
    std::uint32_t original_root_offset;
    std::uint32_t original_slot_index;
    std::uint32_t original_loader_options;

    // Exact floating arguments passed to PPU 0x00105308. Their original source
    // names/semantics are not yet proven, so do not rename them to "scale" etc.
    float original_param1;
    float original_param2;
};

inline constexpr std::uint32_t kArenaModelTableBaseOffset = 0x2D2DC0;
inline constexpr std::uint32_t kArenaModelObjectStride = 0x90;

std::span<const ArenaModelAssetSpec> arena_model_asset_manifest();

}  // namespace comet::decomp
