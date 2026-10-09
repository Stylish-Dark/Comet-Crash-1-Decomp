#include "comet/arena_routes.hpp"
#include <stdexcept>
using namespace comet::decomp;
static void check(bool v) {
  if (!v)
    throw std::runtime_error("route check failed");
}
int main() {
  std::array<std::uint8_t, 576> visibility{};
  auto f = build_arena_route_field(visibility, {12, 12}, 0);
  check(f.costs[12 * 24 + 12] == 0);
  check(f.routes[12 * 24 + 11] == 14);
  check(f.routes[12 * 24 + 13] == 6);
  check(f.routes[11 * 24 + 12] == 2);
  check(f.routes[13 * 24 + 12] == 10);
  check(f.routes[0] == 80 && f.routes[23 * 24 + 23] == 88);
  visibility[12 * 24 + 13] = 1;
  f = build_arena_route_field(visibility, {12, 12}, 0);
  check(f.costs[12 * 24 + 13] == 10001);
  check((f.routes[12 * 24 + 13] & 1) == 1);
  f = build_arena_route_field(visibility, {12, 12}, 1);
  check(f.costs[12 * 24 + 13] == 1);
  check(arena_gate_visibility(0, 0) == 0x1e);
  check(arena_gate_visibility(3, 3) == 0x47);
  bool rejected = false;
  try {
    build_arena_route_field(visibility, {24, 0}, 0);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  check(rejected);
  rejected = false;
  try {
    arena_gate_visibility(4, 0);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  check(rejected);
  std::array<std::uint8_t, 576> initial{};
  initial[0] = 0xab;
  check(build_arena_route_field(visibility, {0, 0}, 0, initial).routes[0] ==
        0xab);
  ArenaRoutingGrid grid;
  std::array<ArenaRoutePlayer, 2> players{
      {{{2, 12}, 0, false}, {{21, 12}, 1, false}}};
  check(apply_arena_grid_placement(grid, 0, 21, {12, 12}, players));
  check(grid.cells[12 * 24 + 12].visibility == 0x1f);
  check(grid.pair_lengths[1] == 21 && grid.pair_lengths[4] == 21);
  auto occupied = grid;
  check(!apply_arena_grid_placement(grid, 0, 21, {12, 12}, players));
  check(grid == occupied);
  check(apply_arena_grid_placement(grid, 0, 255, {12, 12}, players));
  check(grid.cells[12 * 24 + 12].visibility == 0);
  check(grid.pair_lengths[1] == 19);
  for (int z = 0; z < 24; ++z)
    grid.cells[z * 24 + 12].visibility = 0x1f;
  grid.cells[12 * 24 + 12].visibility = 0;
  occupied = grid;
  check(!apply_arena_grid_placement(grid, 0, 21, {12, 12}, players));
  check(grid == occupied);
}
