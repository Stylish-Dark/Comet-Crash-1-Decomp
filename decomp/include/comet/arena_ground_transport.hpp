#pragma once
#include "comet/arena_unit_binding.hpp"
namespace comet::decomp {
// Explicit native transport policy, not the original steering/collision solver.
enum class ArenaGroundTransport { Moved, Blocked, NoTarget, CellFull, Invalid };
ArenaGroundTransport transport_arena_ground_unit(
    ArenaEntityStorage &, std::uint16_t index, float elapsed,
    std::span<const ArenaUnitTargetPlayer>, const ArenaRoutingGrid &,
    ArenaRandom &);
} // namespace comet::decomp
