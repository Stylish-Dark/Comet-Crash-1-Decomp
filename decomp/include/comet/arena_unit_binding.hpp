#pragma once
#include "comet/arena_routes.hpp"
#include "comet/arena_unit_entity.hpp"
namespace comet::decomp {
struct ArenaUnitTargetPlayer {
  std::uint8_t team = 0;
  bool base_available = false;
  std::uint8_t override_target = 255;
};
enum class ArenaGroundUnitBinding { Unsupported, NoTarget, Blocked, Bound };
// FE5D8 12BE0/3068/2CE0 target choice and the ground-unit 14FD8 tail.
// Input is an initialized stage-5 unit in the 24x24 grid. Aircraft are
// excluded. Invalid native inputs preserve the slot and RNG. Valid no-target
// writes +7E=255.
ArenaGroundUnitBinding
bind_arena_ground_unit_template(std::span<std::uint8_t, 256>,
                                std::span<const ArenaUnitTargetPlayer>,
                                const ArenaRoutingGrid &, ArenaRandom &);
struct ArenaGroundUnitTemplate {
  ArenaEntitySlot slot{};
  ArenaGroundUnitBinding binding = ArenaGroundUnitBinding::Unsupported;
  std::uint8_t constructor_result = 0;
};
} // namespace comet::decomp
