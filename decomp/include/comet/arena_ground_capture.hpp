#pragma once
#include "comet/arena_entity_storage.hpp"
#include <span>
namespace comet::decomp {
using ArenaPlayerStatisticsBlock=std::array<std::uint8_t,128>;
enum class ArenaGroundCapture { NotEligible, Captured, CapacityRejected, Unsupported, Invalid, HealthDepleted };
// Ownership/accounting portion of 7E40 for overlapping deployed ground victims
// contacting hostile kind 45. Notification and visual-effect callbacks excluded.
ArenaGroundCapture capture_arena_ground_unit(
    ArenaEntitySlot &victim, const ArenaEntitySlot &peer,
    std::span<const std::uint8_t> teams,
    std::span<ArenaPlayerStatisticsBlock> statistics, bool overlapping);
}
