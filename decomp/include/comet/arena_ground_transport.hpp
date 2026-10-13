#pragma once
#include "comet/arena_ground_steering.hpp"
namespace comet::decomp {
// Explicit native transport policy, not the original steering/collision solver.
enum class ArenaGroundTransport { Moved, Blocked, NoTarget, CellFull, Invalid };
ArenaGroundTransport transport_arena_ground_unit(
    ArenaEntityStorage &, std::uint16_t index, float elapsed,
    std::span<const ArenaUnitTargetPlayer>, const ArenaRoutingGrid &,
    ArenaRandom &, std::span<const ArenaGridPosition> target_bases = {},
    std::span<const std::uint8_t> visibility_bits = {});
} // namespace comet::decomp
