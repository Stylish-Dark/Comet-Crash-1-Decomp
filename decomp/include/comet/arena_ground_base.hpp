#pragma once
#include "comet/arena_state.hpp"
#include <array>
#include <optional>
#include <span>
namespace comet::decomp {
struct ArenaGroundBase {
  ArenaGridPosition cell{};
  std::uint8_t team=0;
  bool available=false;
};
// 6F40..7130 arrival selection: live unit, enemy/available base, strict
// XYZ distance < .5; last eligible table entry wins. Invalid native input
// returns no arrival. Entries correspond to explicit native player indices.
std::optional<std::uint8_t> select_arena_ground_base(
    const std::array<float,4> &position, std::uint8_t team, float health,
    std::span<const ArenaGroundBase> bases);
}
