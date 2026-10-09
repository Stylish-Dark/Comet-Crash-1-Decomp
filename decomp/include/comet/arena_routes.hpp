#pragma once
#include "comet/arena_state.hpp"
namespace comet::decomp {
struct ArenaRouteField {
  std::array<float, 576> costs{};
  // Bit 0: route crosses an obstruction; bits 1..3: predecessor direction;
  // bits 4..7: saturated floor(cost / 3), using the original float constant.
  std::array<std::uint8_t, 576> routes{};
};
// Raw construction job +0xE48. visibility_bit selects a low-nibble team bit.
// The source route byte is preserved, as in the original routine.
ArenaRouteField build_arena_route_field(
    std::span<const std::uint8_t, 576> visibility, ArenaGridPosition source,
    std::uint8_t visibility_bit,
    const std::array<std::uint8_t, 576> &initial_routes = {});
// Raw construction job +0x9E8 gate mask, before route validation/commit.
std::uint8_t arena_gate_visibility(std::uint8_t player, std::uint8_t team_bit);
struct ArenaRoutingCell {
  std::uint16_t route_usage = 0;
  std::uint8_t reserved = 0, visibility = 0;
  std::array<std::uint8_t, 12> routes{};
  bool operator==(const ArenaRoutingCell &) const = default;
};
struct ArenaRoutingGrid {
  std::array<ArenaRoutingCell, 576> cells{};
  std::array<std::uint32_t, 32> pair_lengths{};
  bool operator==(const ArenaRoutingGrid &) const = default;
};
struct ArenaRoutePlayer {
  ArenaGridPosition base;
  std::uint32_t team = 0;
  bool inactive = false;
};
// Transactional grid portion of the raw construction job. Opcode 0's atomic
// cell-list removal and opcode 29's base relocation remain separate boundaries.
// A rejected candidate leaves the supplied grid intact.
bool apply_arena_grid_placement(ArenaRoutingGrid &, std::uint8_t owner,
                                std::uint8_t opcode, ArenaGridPosition,
                                std::span<const ArenaRoutePlayer>);
} // namespace comet::decomp
