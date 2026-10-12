#pragma once
#include "comet/arena_entity_storage.hpp"
#include <optional>
namespace comet::decomp {
struct ArenaUnitProductionQuota {
  std::int16_t active = 0;
  std::uint16_t pending = 0;
  std::uint32_t capacity = 0;
  std::array<std::uint16_t, 9> by_type{};
};
// BA08 quota claim core: promote one reserved kind to active, or return empty.
// This is separate from live allocation and retains original halfword wrap.
std::optional<std::uint8_t>
claim_arena_unit_production(ArenaUnitProductionQuota &, std::uint8_t kind);
enum class ArenaFactoryProduction {
  Blocked,
  Progress,
  Reserved,
  Full,
  Unsupported
};
// FE5D8 F5B8..F858 timer and C238..C378 quota reservation core.
// Reserves pending production only; live-unit allocation is a separate job.
ArenaFactoryProduction
advance_arena_factory_production(std::span<std::uint8_t, 256>, float elapsed,
                                 bool enabled, ArenaUnitProductionQuota &);
} // namespace comet::decomp
