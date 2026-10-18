#pragma once
#include "comet/arena_entity_storage.hpp"
#include "comet/arena_routes.hpp"
namespace comet::decomp {
// Shared 4428/4078 local-first target priority, bounded to ordinary stage5
// ground records. Both Tesla mask62 and turret mask126 admit this subset.
std::optional<std::uint16_t> select_arena_ground_weapon_target(
 const ArenaEntityStorage&,const ArenaRoutingGrid&,const ArenaEntitySlot&,
 std::span<const std::uint8_t> teams);
}
