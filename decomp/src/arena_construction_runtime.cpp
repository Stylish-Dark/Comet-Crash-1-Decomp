#include "comet/arena_construction_runtime.hpp"
namespace comet::decomp {
ArenaConstructionStep process_next_arena_construction(
    ArenaConstructionQueue &queue, ArenaRoutingGrid &grid,
    std::span<ArenaRoutePlayer> route_players, std::span<ArenaPlayer> players,
    ArenaConstructionContext &context,
    const std::function<bool(ArenaConstructionRequest)> &construct) {
  if (queue.empty())
    return ArenaConstructionStep::Idle;
  const auto request = queue.pop();
  if (route_players.empty() || route_players.size() > 4 ||
      players.size() != route_players.size() ||
      request.player >= players.size() || request.grid_x >= 24 ||
      request.grid_z >= 24 ||
      (request.opcode != 255 && !arena_construction_cost(request.opcode)))
    return ArenaConstructionStep::InvalidRequest;
  const ArenaGridPosition location{request.grid_x, request.grid_z};
  if (!apply_arena_grid_placement(grid, request.player, request.opcode,
                                  location, route_players))
    return ArenaConstructionStep::RouteRejected;
  if (request.opcode == 255)
    return ArenaConstructionStep::Released;
  // The completed SPU job has already reserved the visibility nibble. CCD20
  // supplies occupancy override; CCD24 disables the one-unit cost override.
  struct RestoreFlags {
    ArenaConstructionContext &context;
    bool occupancy, cost;
    ~RestoreFlags() {
      context.override_occupancy = occupancy;
      context.cost_override = cost;
    }
  } restore{context, context.override_occupancy, context.cost_override};
  context.visibility =
      grid.cells[request.grid_z * 24 + request.grid_x].visibility;
  context.override_occupancy = true;
  context.cost_override = false;
  if (!commit_arena_construction(request, context, queue, construct)) {
    const ArenaConstructionRequest release{request.player, 255, request.grid_x,
                                           request.grid_z};
    return queue.push(release) ? ArenaConstructionStep::ReleaseQueued
                               : ArenaConstructionStep::ReleaseQueueFull;
  }
  if (request.opcode == 29) {
    auto &route_player = route_players[request.player];
    route_player.base = location;
    route_player.inactive = false;
    auto &player = players[request.player];
    player.start_grid = location;
    player.control_23a = 1;
    player.control_42 = player.control_43 = 0;
  }
  return ArenaConstructionStep::Constructed;
}
} // namespace comet::decomp
