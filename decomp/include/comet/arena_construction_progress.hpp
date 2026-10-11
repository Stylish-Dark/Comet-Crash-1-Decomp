#pragma once
#include "comet/arena_entity_storage.hpp"
namespace comet::decomp {
enum class ArenaConstructionProgress { Idle, Unsupported, Building, Ready };
// FE5D8 construction arm 4DDC..4F18 and completion leaf 1FEE8..1FFB4.
// Updates the core only. Original transient effect/notification allocation is
// outside this boundary. Unsupported slots are retained without modification.
ArenaConstructionProgress
advance_arena_structure_construction(std::span<std::uint8_t, 256> slot,
                                     float elapsed);
bool arena_structure_ready(std::span<const std::uint8_t, 256> slot);
} // namespace comet::decomp
