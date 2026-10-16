#pragma once
#include "comet/arena_entity_storage.hpp"
#include "comet/arena_routes.hpp"
namespace comet::decomp {
struct ArenaTeslaTick {
 std::vector<std::uint16_t> spawned,retired;
};
// Internal native scheduling adapter. Inputs are the caller's staged world,
// RNG, health and command lanes; caller commits only after the whole tick.
ArenaTeslaTick stage_arena_teslas(ArenaEntityStorage &,const ArenaRoutingGrid &,
 std::span<const std::uint8_t> teams,float elapsed,ArenaRandom &,
 std::span<float> health,std::span<std::uint32_t> commands);
}
