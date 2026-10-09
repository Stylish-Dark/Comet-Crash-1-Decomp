#include "comet/arena_routes.hpp"
#include "comet/arena_construction.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace comet::decomp {
ArenaRouteField
build_arena_route_field(std::span<const std::uint8_t, 576> visibility,
                        ArenaGridPosition source, std::uint8_t visibility_bit,
                        const std::array<std::uint8_t, 576> &initial_routes) {
  if (source.x >= 24 || source.z >= 24 || visibility_bit >= 4)
    throw std::invalid_argument("invalid arena route query");
  constexpr float infinity = std::bit_cast<float>(0x5368d4a5u);
  constexpr float selection_limit = std::bit_cast<float>(0x551184e7u);
  constexpr float diagonal_cost = std::bit_cast<float>(0x3fb504f3u);
  constexpr float quantization = std::bit_cast<float>(0x3eaaaaaau);
  constexpr std::array<int, 8> dx{-1, 0, 1, 1, 1, 0, -1, -1};
  constexpr std::array<int, 8> dz{-1, -1, -1, 0, 1, 1, 1, 0};
  ArenaRouteField result;
  result.costs.fill(infinity);
  result.routes = initial_routes;
  result.costs[source.z * 24 + source.x] = 0;
  std::array<bool, 576> visited{};
  for (;;) {
    // +0xC78 scans in ascending cell order and uses strict comparisons.
    float smallest = selection_limit;
    int current = -1;
    for (int i = 0; i < 576; ++i) {
      if (!visited[i] && result.costs[i] < smallest) {
        smallest = result.costs[i];
        current = i;
      }
    }
    if (current < 0)
      break;
    visited[current] = true;
    const int x = current % 24, z = current / 24;
    std::array<float, 8> obstruction{};
    std::array<int, 8> neighbor{};
    for (int direction = 0; direction < 8; ++direction) {
      const int nx = x + dx[direction], nz = z + dz[direction];
      // The original boundary table uses offset zero for absent neighbors.
      neighbor[direction] =
          nx < 0 || nx >= 24 || nz < 0 || nz >= 24 ? current : nz * 24 + nx;
      obstruction[direction] =
          (visibility[neighbor[direction]] & (1u << visibility_bit)) ? 10000.0f
                                                                     : 0.0f;
    }
    auto relax = [&](int direction, float cost) {
      const auto target = neighbor[direction];
      if (!(result.costs[target] > cost))
        return;
      result.costs[target] = cost;
      const float scaled = cost * quantization;
      const auto band = scaled >= 15.0f ? 15u : static_cast<unsigned>(scaled);
      result.routes[target] = static_cast<std::uint8_t>(
          (band << 4) | (direction << 1) | (cost >= 10000.0f ? 1 : 0));
    };
    // +0x10A0 cardinal updates precede +0x1200 diagonal updates. Preserve
    // their addition order: diagonal corner penalties are added separately.
    for (int direction = 1; direction < 8; direction += 2)
      relax(direction, (obstruction[direction] + smallest) + 1.0f);
    for (int direction = 0; direction < 8; direction += 2) {
      const float corner = obstruction[(direction + 7) % 8] + diagonal_cost;
      const float corners = corner + obstruction[direction + 1];
      const float destination = obstruction[direction] + smallest;
      relax(direction, corners + destination);
    }
  }
  return result;
}
std::uint8_t arena_gate_visibility(std::uint8_t player, std::uint8_t team_bit) {
  if (player >= 4 || team_bit >= 4)
    throw std::invalid_argument("invalid gate owner or team");
  return static_cast<std::uint8_t>(((player + 1) << 4) |
                                   (std::rotl(0xfffffffeu, team_bit) & 15));
}
static bool rebuild_arena_routes(ArenaRoutingGrid &candidate,
                                 std::span<const ArenaRoutePlayer> players) {
  // +0x3D0 clears usage masks even for inactive/same-team pairs. Other cell
  // fields and inactive pair route bytes/lengths retain their previous values.
  std::array<std::uint8_t, 576> visibility{};
  for (std::size_t n = 0; n < 576; ++n) {
    candidate.cells[n].route_usage = 0;
    visibility[n] = candidate.cells[n].visibility;
  }
  constexpr std::array<int, 8> dx{1, 0, -1, -1, -1, 0, 1, 1};
  constexpr std::array<int, 8> dz{1, 1, 1, 0, -1, -1, -1, 0};
  for (std::size_t i = 0; i < players.size(); ++i) {
    if (players[i].inactive)
      continue;
    for (std::size_t j = 0; j < players.size(); ++j) {
      if (j == i || players[j].inactive || players[i].team == players[j].team)
        continue;
      const auto pair = 3 * i + (j > i ? j - 1 : j);
      std::array<std::uint8_t, 576> initial{};
      for (std::size_t n = 0; n < 576; ++n)
        initial[n] = candidate.cells[n].routes[pair];
      const auto field = build_arena_route_field(visibility, players[j].base,
                                                 players[i].team, initial);
      for (std::size_t n = 0; n < 576; ++n)
        candidate.cells[n].routes[pair] = field.routes[n];
      int x = players[i].base.x, z = players[i].base.z;
      if (field.routes[z * 24 + x] & 1)
        return false;
      const auto usage = static_cast<std::uint16_t>(1u << (i + 4 * j));
      std::uint32_t length = 0;
      auto mark = [&](int mx, int mz) {
        if (mx < 0 || mx >= 24 || mz < 0 || mz >= 24)
          throw std::runtime_error("arena route escaped grid");
        candidate.cells[mz * 24 + mx].route_usage |= usage;
      };
      std::size_t steps = 0;
      while (x != players[j].base.x || z != players[j].base.z) {
        if (++steps > 576)
          throw std::runtime_error("cyclic arena route");
        const auto direction = (field.routes[z * 24 + x] >> 1) & 7;
        mark(x, z);
        // +0x6B0..0x764 marks both adjacent cardinal cells for a diagonal.
        if ((direction & 1) == 0) {
          mark(x + dx[direction], z);
          mark(x, z + dz[direction]);
        }
        length += (dx[direction] != 0) + (dz[direction] != 0);
        x += dx[direction];
        z += dz[direction];
      }
      candidate.pair_lengths[4 * i + j] = length;
    }
  }
  return true;
}
bool apply_arena_grid_placement(ArenaRoutingGrid &grid, std::uint8_t owner,
                                std::uint8_t opcode, ArenaGridPosition location,
                                std::span<const ArenaRoutePlayer> players) {
  if (owner >= players.size() || players.size() > 4 || location.x >= 24 ||
      location.z >= 24 || (opcode != 255 && !arena_construction_cost(opcode)))
    throw std::invalid_argument("unsupported arena grid command");
  for (const auto &player : players)
    if (player.team >= 4 ||
        (!player.inactive && (player.base.x >= 24 || player.base.z >= 24)))
      throw std::invalid_argument("invalid arena route player");
  std::array<ArenaRoutePlayer, 4> updated_players{};
  std::copy(players.begin(), players.end(), updated_players.begin());
  const auto player_count = players.size();
  auto candidate = grid;
  auto &cell = candidate.cells[location.z * 24 + location.x];
  if (opcode == 255) {
    if ((cell.visibility >> 4) == owner + 1)
      cell.visibility = 0;
  } else {
    if ((cell.visibility & 0xf0) != 0)
      return false;
    cell.visibility = opcode == 25
                          ? arena_gate_visibility(owner, players[owner].team)
                          : static_cast<std::uint8_t>(((owner + 1) << 4) |
                                                      (opcode == 29 ? 0 : 15));
    if (opcode == 29) {
      updated_players[owner].base = location;
      updated_players[owner].inactive = false;
      players = std::span<const ArenaRoutePlayer>(updated_players.data(),
                                                  player_count);
    }
  }
  if (!rebuild_arena_routes(candidate, players))
    return false;
  grid = candidate;
  return true;
}
bool clear_arena_grid_cell_list(ArenaRoutingGrid &grid,
                                std::vector<ArenaGridPosition> &coordinates,
                                std::span<const ArenaRoutePlayer> players) {
  if (coordinates.size() > 63 || players.size() > 4)
    throw std::invalid_argument("invalid arena cell list");
  for (const auto &position : coordinates)
    if (position.x >= 24 || position.z >= 24)
      throw std::invalid_argument("invalid arena cell list coordinate");
  for (const auto &player : players)
    if (player.team >= 4 ||
        (!player.inactive && (player.base.x >= 24 || player.base.z >= 24)))
      throw std::invalid_argument("invalid arena route player");
  auto candidate = grid;
  // +0xBA8 atomically takes the 128-byte list and zeroes its count before
  // +0x8E8 clears visibility. That side effect precedes route rejection.
  auto removed = std::move(coordinates);
  coordinates.clear();
  for (const auto &position : removed)
    candidate.cells[position.z * 24 + position.x].visibility = 0;
  if (!rebuild_arena_routes(candidate, players))
    return false;
  grid = candidate;
  return true;
}
} // namespace comet::decomp
