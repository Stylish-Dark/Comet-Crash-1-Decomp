#pragma once
#include "comet/arena_entity_storage.hpp"
#include <optional>
namespace comet::decomp {
// Return value published by the complete original 14B30 constructor in r3.
// Its consumer-specific meaning is kept separate from the model slot.
std::optional<std::uint8_t> arena_unit_constructor_result(std::uint8_t kind);
// FE5D8 14B30 prefix through 14FD8, before world-dependent cell binding.
// Preserves untouched bytes; consumes one original RNG value on success.
bool initialize_arena_unit_template(std::span<std::uint8_t, 256>,
                                    std::uint8_t kind, std::uint8_t owner,
                                    const std::array<float, 4> &position,
                                    ArenaRandom &);
} // namespace comet::decomp
