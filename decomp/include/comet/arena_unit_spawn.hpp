#pragma once
#include "comet/arena_player_statistics.hpp"
#include "comet/arena_unit_binding.hpp"
namespace comet::decomp {
// C380 position jitter followed by the complete ground-unit constructor.
// Invalid native inputs preserve slot and RNG.
std::optional<ArenaGroundUnitTemplate> initialize_arena_ground_unit_spawn(
    std::span<const std::uint8_t, 256> reused_slot, std::uint8_t kind,
    std::uint8_t owner, const std::array<float, 4> &position,
    std::span<const ArenaUnitTargetPlayer>, const ArenaRoutingGrid &,
    ArenaRandom &);
enum class ArenaUnitSpawnStatus {
  Spawned,
  NoOrder,
  Invalid,
  CapacityRejected,
  AllocationFailed
};
struct ArenaUnitSpawnResult {
  ArenaUnitSpawnStatus status = ArenaUnitSpawnStatus::Invalid;
  std::optional<std::uint16_t> entity_index;
  ArenaGroundUnitBinding binding = ArenaGroundUnitBinding::Unsupported;
};
// Native single-spawn adapter around recovered claim/constructor/registration.
// Commits both owned banks and actual-cell lists, using the unit model marker.
ArenaUnitSpawnResult spawn_reserved_arena_ground_unit(
    ArenaEntityStorage &, std::span<std::uint8_t, 128> statistics,
    std::uint8_t kind, std::uint8_t owner, const std::array<float, 4> &position,
    std::span<const ArenaUnitTargetPlayer>, const ArenaRoutingGrid &,
    ArenaRandom &);
} // namespace comet::decomp
