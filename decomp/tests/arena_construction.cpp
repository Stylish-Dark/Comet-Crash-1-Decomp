#include "comet/arena_construction.hpp"
#include <stdexcept>
using namespace comet::decomp;
void check(bool value) {
  if (!value)
    throw std::runtime_error("construction check failed");
}
int main() {
  check(arena_construction_cost(9) == 1);
  check(arena_construction_cost(20) == 20);
  check(arena_construction_cost(28) == 150);
  check(arena_construction_cost(29) == 0);
  check(!arena_construction_cost(27));
  ArenaConstructionQueue queue;
  ArenaConstructionRequest request{0, 26, 2, 3};
  ArenaConstructionContext context;
  context.available_entities = 1;
  context.resource = 60;
  context.resource_max = 100;
  check(validate_arena_construction(request, context, queue) ==
        ArenaConstructionError::None);
  auto blocked = context;
  blocked.visibility = 0x1f;
  check(validate_arena_construction(request, blocked, queue) ==
        ArenaConstructionError::Occupied);
  blocked.override_occupancy = true;
  check(validate_arena_construction(request, blocked, queue) ==
        ArenaConstructionError::None);
  blocked = context;
  blocked.cell_entry_count = 126;
  check(validate_arena_construction(request, blocked, queue) ==
        ArenaConstructionError::CellFull);
  blocked = context;
  blocked.available_entities = 0;
  check(validate_arena_construction(request, blocked, queue) ==
        ArenaConstructionError::NoEntityCapacity);
  blocked = context;
  blocked.resource = 59.9f;
  check(validate_arena_construction(request, blocked, queue) ==
        ArenaConstructionError::InsufficientResources);
  blocked.cost_override = true;
  check(validate_arena_construction(request, blocked, queue) ==
        ArenaConstructionError::None);
  check(queue.push(request));
  auto other = request;
  other.player = 1;
  other.opcode = 21;
  check(validate_arena_construction(other, context, queue) ==
        ArenaConstructionError::Duplicate);
  check(queue.pop().opcode == 26);
  check(queue.empty());
  for (int i = 0; i < 256; ++i)
    check(queue.push({0, 21, static_cast<std::uint8_t>(i), 0}));
  check(!queue.push(request));
  for (int i = 0; i < 256; ++i)
    check(queue.pop().grid_x == i);
  auto balance = context.resource;
  check(!commit_arena_construction(request, context, queue,
                                   [](auto) { return false; }));
  check(context.resource == balance);
  check(commit_arena_construction(request, context, queue,
                                  [](auto) { return true; }));
  check(context.resource == 0);
  auto changed_flags = context;
  changed_flags.resource = 60;
  check(commit_arena_construction(request, changed_flags, queue, [&](auto) {
    changed_flags.cost_override = true;
    return true;
  }));
  check(changed_flags.resource == 0);
  request.opcode = 29;
  check(commit_arena_construction(request, context, queue,
                                  [](auto) { return true; }));
  check(context.resource == 0);
  request.grid_x = 24;
  check(validate_arena_construction(request, context, queue) ==
        ArenaConstructionError::InvalidRequest);
}
