#pragma once
#include "comet/arena_factory_production.hpp"
#include <span>
namespace comet::decomp {
// Typed inputs to FE5D8 BB30. Names retain original decimal byte offsets.
struct ArenaPlayerStatisticsDelta {
  std::array<float, 16> scalars_00_to_60{};
  float ground_weight = 0, air_weight = 0;
  std::int16_t active = 0, count_86 = 0;
  std::uint8_t count_92 = 0, count_93 = 0, auxiliary_122 = 0;
  float scalar_116 = 0;
};
// False skips active/weighted writes, but still applies the other deltas.
// Unsupported floats throw invalid_argument before any writes.
bool apply_arena_player_statistics_delta(std::span<std::uint8_t, 128>,
                                         const ArenaPlayerStatisticsDelta &);
enum class ArenaEntityRegistration {
  Registered,
  CapacityRejected,
  UnsupportedEntity
};
ArenaEntityRegistration register_arena_entity_statistics(
    std::span<std::uint8_t, 128>, std::span<const std::uint8_t, 256>,
    bool adding, bool count_active, std::uint8_t auxiliary = 0);
// Bridge the raw statistics block to the existing reservation/claim leaf.
ArenaUnitProductionQuota
    read_arena_unit_production_quota(std::span<const std::uint8_t, 128>);
void store_arena_unit_production_quota(std::span<std::uint8_t, 128>,
                                       const ArenaUnitProductionQuota &);
} // namespace comet::decomp
