#pragma once
#include "comet/arena_entity_storage.hpp"
namespace comet::decomp {
enum class ArenaFactorySelection {
  Changed,
  Unchanged,
  Unsupported,
  NoTarget,
  InvalidRequest
};
// PPU CDA88..CDA98 and CCC04..CCC24: selectors 4..8 select mode 0..4
// on types 26/28. Reselecting the current mode preserves the timer at +80.
ArenaFactorySelection
select_arena_factory_production(std::span<std::uint8_t, 256> slot,
                                std::uint8_t selector);
} // namespace comet::decomp
