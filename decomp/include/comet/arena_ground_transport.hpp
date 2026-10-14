#pragma once
#include "comet/arena_ground_steering.hpp"
#include "comet/arena_ground_capture.hpp"
namespace comet::decomp {
// Explicit native transport policy, not the original steering/collision solver.
enum class ArenaGroundTransport { Moved, Blocked, NoTarget, CellFull, Invalid };
ArenaGroundTransport transport_arena_ground_unit(
    ArenaEntityStorage &, std::uint16_t index, float elapsed,
    std::span<const ArenaUnitTargetPlayer>, const ArenaRoutingGrid &,
    ArenaRandom &, std::span<const ArenaGridPosition> target_bases = {},
    std::span<const std::uint8_t> visibility_bits = {},
    bool original_velocity_dynamics = false,
    bool motion_contacts = false,
    std::span<ArenaPlayerStatisticsBlock> capture_statistics = {});
} // namespace comet::decomp
