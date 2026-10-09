#include "comet/arena_construction_runtime.hpp"
#include "comet/arena_entity_storage.hpp"
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
  std::array<std::uint8_t, 256> entity_slot{};
  ArenaEntityStorage storage;
  storage.banks[0].resize(2);
  storage.banks[1].resize(2);
  reset_arena_entity_index_pool(storage, 2);
  context.available_entities =
      static_cast<std::uint32_t>(storage.free_indices.size());
  auto construct = [&](auto request) {
    ++calls;
    check(request.opcode == 21);
    initialize_arena_type21_entity(entity_slot, request.player,
                                   {request.grid_x, request.grid_z},
                                   {50, 10, 25, 10});
    const auto index = insert_arena_entity(
        storage, {request.grid_x, request.grid_z}, entity_slot);
    context.available_entities =
        static_cast<std::uint32_t>(storage.free_indices.size());
    return index.has_value();
  };
  check(queue.push({0, 21, 8, 12}));
  check(process_next_arena_construction(queue, grid, routes, players, context,
                                        construct) ==
        ArenaConstructionStep::Constructed);
  check(calls == 1 && context.resource == 90 &&
        grid.cells[12 * 24 + 8].visibility == 0x1f);
  check(!context.override_occupancy && queue.empty());
  check(entity_slot[0xf] == 21 && entity_slot[0x6f] == 11 &&
        entity_slot[0x33] == 0x10);
  check(storage.banks[0][0][0xf] == 21 && storage.banks[1][0][0xf] == 21);
  check(storage.cells[0][12 * 24 + 8].size() == 1 &&
        storage.cells[1][12 * 24 + 8].size() == 1);
  check(storage.summary_counts[12 * 24 + 8] == 1 &&
        context.available_entities == 1);
  // Construct every recovered structure through route reservation, completion,
  // allocation and both cell banks, rather than testing only leaf writes.
  {
    ArenaConstructionQueue batch;
    ArenaRoutingGrid batch_grid;
    std::array<ArenaRoutePlayer, 2> batch_routes{
        {{{2, 12}, 0, false}, {{21, 12}, 1, false}}};
    std::array<ArenaPlayer, 2> batch_players;
    ArenaEntityStorage batch_storage;
    batch_storage.banks[0].resize(8);
    batch_storage.banks[1].resize(8);
    reset_arena_entity_index_pool(batch_storage, 8);
    ArenaConstructionContext batch_context;
    batch_context.available_entities = 8;
    batch_context.resource = batch_context.resource_max = 400;
    constexpr std::array<std::uint8_t, 8> opcodes{20, 21, 22, 23, 24, 25, 26, 28};
    for (unsigned i = 0; i < opcodes.size(); ++i)
      check(batch.push({0, opcodes[i], static_cast<std::uint8_t>(4 + i), 8}));
    unsigned allocated = 0;
    ArenaRandom batch_random(1);
    auto allocate = [&](auto request) {
      ArenaEntitySlot slot;
      slot.fill(0xa5);
      check(initialize_arena_structure_entity(
          slot, request.opcode, request.player,
          {request.grid_x, request.grid_z}, {100, 50, 10, 20, 7.5f}, &batch_random));
      const auto id = insert_arena_entity(
          batch_storage, {request.grid_x, request.grid_z}, slot);
      check(id.has_value() && *id == allocated++);
      batch_context.available_entities =
          static_cast<std::uint32_t>(batch_storage.free_indices.size());
      return true;
    };
    for (unsigned i = 0; i < opcodes.size(); ++i) {
      check(process_next_arena_construction(
                batch, batch_grid, batch_routes, batch_players, batch_context,
                allocate) == ArenaConstructionStep::Constructed);
      const auto cell = 8 * 24 + 4 + i;
      check(batch_storage.banks[0][i][0xf] == opcodes[i]);
      check(batch_storage.banks[0][i] == batch_storage.banks[1][i]);
      check(batch_storage.cells[0][cell].size() == 1 &&
            batch_storage.cells[1][cell].size() == 1);
      check(batch_storage.summary_counts[cell] == 1 &&
            batch_grid.cells[cell].visibility == (opcodes[i] == 25 ? 0x1e : 0x1f));
    }
    check(batch.empty() && allocated == 8 && batch_context.resource == 35);
    check(batch_context.available_entities == 0 &&
          batch_storage.free_indices.empty());
  }
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
  context.resource = 1;
  context.cost_override = true;
  check(queue.push({0, 21, 9, 12}));
  check(process_next_arena_construction(queue, grid, routes, players, context,
                                        construct) ==
        ArenaConstructionStep::ReleaseQueued);
  check(context.resource == 1 && context.cost_override && calls == 1);
  check(process_next_arena_construction(queue, grid, routes, players, context,
                                        construct) ==
        ArenaConstructionStep::Released);
  context.resource = 90;
  context.cost_override = false;
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
