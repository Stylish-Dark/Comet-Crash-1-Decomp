#include "comet/arena_construction_runtime.hpp"
#include "comet/arena_structure_entity.hpp"
#include <stdexcept>
using namespace comet::decomp;
static void check(bool v) {
  if (!v)
    throw std::runtime_error("construction runtime check failed");
}
int main() {
  ArenaConstructionQueue queue;
  ArenaRoutingGrid grid;
  std::array<ArenaRoutePlayer, 2> routes{
      {{{2, 12}, 0, false}, {{21, 12}, 1, false}}};
  std::array<ArenaPlayer, 2> players;
  ArenaConstructionContext context;
  context.available_entities = 1;
  context.resource = context.resource_max = 100;
  unsigned calls = 0;
  std::array<std::uint8_t,256> entity_slot{};
  auto construct = [&](auto request) {
    ++calls;
    check(request.opcode == 21);
    initialize_arena_type21_entity(entity_slot,request.player,
        {request.grid_x,request.grid_z},{50,10,25,10});
    return true;
  };
  check(queue.push({0, 21, 8, 12}));
  check(process_next_arena_construction(queue, grid, routes, players, context,
                                        construct) ==
        ArenaConstructionStep::Constructed);
  check(calls == 1 && context.resource == 90 &&
        grid.cells[12 * 24 + 8].visibility == 0x1f);
  check(!context.override_occupancy && queue.empty());
  check(entity_slot[0xf]==21 && entity_slot[0x6f]==11 && entity_slot[0x33]==0x10);
  // An entity failure leaves the reservation until a queued 255 job completes.
  check(queue.push({0, 21, 9, 12}));
  check(process_next_arena_construction(queue, grid, routes, players, context,
                                        [](auto) { return false; }) ==
        ArenaConstructionStep::ReleaseQueued);
  check(context.resource == 90 && grid.cells[12 * 24 + 9].visibility == 0x1f &&
        queue.size() == 1);
  check(process_next_arena_construction(queue, grid, routes, players, context,
                                        construct) ==
        ArenaConstructionStep::Released);
  check(calls == 1 && context.resource == 90 &&
        grid.cells[12 * 24 + 9].visibility == 0);
  // Completion uses the full opcode cost even when the caller context had a
  // one-unit cost override enabled. Insufficient funds unwind the reservation.
  context.resource=1;
  context.cost_override=true;
  check(queue.push({0,21,9,12}));
  check(process_next_arena_construction(queue,grid,routes,players,context,construct)==ArenaConstructionStep::ReleaseQueued);
  check(context.resource==1 && context.cost_override && calls==1);
  check(process_next_arena_construction(queue,grid,routes,players,context,construct)==ArenaConstructionStep::Released);
  context.resource=90;
  context.cost_override=false;
  // Base completion persists routing state without clearing queued events.
  players[0].events.push({3, 4, 5, 0, 21});
  players[0].control_42 = 7;
  players[0].control_43 = 9;
  routes[0].inactive = true;
  check(queue.push({0, 29, 3, 11}));
  check(process_next_arena_construction(queue, grid, routes, players, context,
                                        [](auto) { return true; }) ==
        ArenaConstructionStep::Constructed);
  check(routes[0].base == ArenaGridPosition{3, 11} && !routes[0].inactive);
  check(players[0].start_grid == ArenaGridPosition{3, 11});
  check(players[0].control_23a == 1 && players[0].control_42 == 0 &&
        players[0].control_43 == 0);
  check(players[0].events.size() == 1 && players[0].events.next().time == 3);
  // A rejected route never reaches the entity or resource boundary.
  for (unsigned z = 0; z < 24; ++z)
    grid.cells[z * 24 + 12].visibility = 0x1f;
  check(queue.push({0, 21, 5, 7}));
  auto before = grid;
  check(process_next_arena_construction(queue, grid, routes, players, context,
                                        construct) ==
        ArenaConstructionStep::RouteRejected);
  check(grid == before && calls == 1 && context.resource == 90);
  check(process_next_arena_construction(queue, grid, routes, players, context,
                                        construct) ==
        ArenaConstructionStep::Idle);
}
