#pragma once
#include "comet/arena_entity_storage.hpp"
#include "comet/arena_routes.hpp"
namespace comet::decomp {
struct ArenaCannonTick {
  std::vector<std::uint16_t> spawned, retired, impacts;
};
// Operates on the simulation's staged transaction. Ground targets only.
ArenaCannonTick stage_arena_cannons(ArenaEntityStorage &, const ArenaRoutingGrid &,
    std::span<const ArenaRoutePlayer>, float elapsed, ArenaRandom &,
    std::span<std::array<float,4>> impulses);
}
