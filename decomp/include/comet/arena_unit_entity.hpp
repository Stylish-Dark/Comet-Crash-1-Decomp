#pragma once
#include "comet/arena_entity_storage.hpp"
namespace comet::decomp {
// FE5D8 14B30 prefix through 14FD8, before world-dependent cell binding.
// Preserves untouched bytes; consumes one original RNG value on success.
bool initialize_arena_unit_template(std::span<std::uint8_t, 256>,
                                    std::uint8_t kind, std::uint8_t owner,
                                    const std::array<float, 4> &position,
                                    ArenaRandom &);
} // namespace comet::decomp
