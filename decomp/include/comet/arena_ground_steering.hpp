#pragma once
#include "comet/arena_unit_binding.hpp"
namespace comet::decomp {
struct ArenaGroundSteeringQuery {
  std::array<float,4> position{}, orientation{0,0,0,1};
  std::uint8_t owner=0, target=255, extent=24, visibility_bit=0;
  ArenaGridPosition target_base{};
  bool target_available=false, direct_route=false, initial_blocked=false;
  // Original auxiliary halfwords +48/+4A suppress one candidate cell.
  std::array<std::int16_t,2> suppressed_cell{-1,-1};
};
struct ArenaGroundSteeringResult {
  std::array<float,4> direction{};
  bool blocked=false;
  std::optional<std::uint8_t> route_direction;
};
// Complete FE5D8 13998..1405C leaf: no RNG, allocation or entity mutation.
// Rejects malformed native query inputs; original callers supplied job context.
std::optional<ArenaGroundSteeringResult> steer_arena_ground_unit(
    const ArenaGroundSteeringQuery &, const ArenaRoutingGrid &);
} // namespace comet::decomp
