#include "comet/arena_construction_simulation.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
bool supported(unsigned type) {
  return (type >= 20 && type <= 26) || type == 28;
}
void scalar(ArenaEntitySlot &s, unsigned off, float f) {
  auto word = std::bit_cast<std::uint32_t>(f);
  for (unsigned i = 0; i < 4; ++i)
    s[off + i] = word >> (24 - i * 8);
}
} // namespace
bool native_map_structure_importable(const ArenaEntity &e) {
  auto level = arena_structure_initial_level(e.type);
  return level && e.initial_upgrade <= *level;
}
ArenaConstructionSimulation::ArenaConstructionSimulation(
    const ArenaConstructionSimulationConfig &c)
    : config_(c), random_(c.seed) {
  if (c.players.empty() || c.players.size() > 4 || c.capacity > 65536)
    throw std::invalid_argument("invalid simulation dimensions");
  for (const auto &a : c.players) {
    if (a.route.base.x >= 24 || a.route.base.z >= 24 || a.route.team > 3 ||
        !std::isfinite(a.resource) || !std::isfinite(a.resource_max) ||
        a.resource < 0 || a.resource_max < a.resource)
      throw std::invalid_argument("invalid player account");
    routes_.push_back(a.route);
    ArenaPlayer p;
    p.start_grid = a.route.base;
    players_.push_back(p);
  }
  if (!c.initial_players.empty()) {
    if (c.initial_players.size() != players_.size())
      throw std::invalid_argument("event player count mismatch");
    players_ = c.initial_players;
    for (const auto &p : players_) {
      auto events = p.events;
      if (events.size() > p.event_capacity)
        throw std::invalid_argument("event heap exceeds capacity");
      while (!events.empty()) {
        auto e = events.pop();
        if (!std::isfinite(e.time) ||
            (e.selector < 9 && (e.grid_x >= 24 || e.grid_z >= 24)))
          throw std::invalid_argument("invalid event cell");
      }
      if (p.command_mode != 11 &&
          (p.pending.selector != p.command_mode ||
           (p.pending.selector < 9 &&
            (p.pending.grid_x >= 24 || p.pending.grid_z >= 24))))
        throw std::invalid_argument("invalid pending command");
    }
  }
  for (unsigned op = 0; op < 32; ++op) {
    const auto &t = c.tuning[op];
    if (t && (!supported(op) || t->scalar_38_denominator <= 0 ||
              !std::isfinite(t->scalar_84)))
      throw std::invalid_argument("invalid tuning");
  }
  if (c.initial_structures.size() > c.capacity)
    throw std::invalid_argument("map exceeds capacity");
  std::array<unsigned, 576> counts{};
  auto visibility = c.initial_visibility;
  for (const auto &e : c.initial_structures) {
    if (!native_map_structure_importable(e) || e.owner >= players_.size() ||
        e.grid.x >= 24 || e.grid.z >= 24 || !c.tuning[e.type] ||
        e.model_slot != *initial_model_slot_for_record(e.type) ||
        e.position.x != e.grid.x + .5f || e.position.y != 0 ||
        e.position.z != e.grid.z + .5f ||
        ++counts[e.grid.z * 24 + e.grid.x] > 126)
      throw std::invalid_argument("invalid initial structure");
    for (float f : e.orientation)
      if (!std::isfinite(f))
        throw std::invalid_argument("invalid orientation");
    auto flat = e.grid.z * 24 + e.grid.x;
    auto supplied = c.initial_visibility[flat];
    if (supplied && !(supplied & 0xf0))
      throw std::invalid_argument("unowned initial occupancy");
    if (!supplied)
      visibility[flat] =
          e.type == 25 ? arena_gate_visibility(e.owner, routes_[e.owner].team)
                       : static_cast<std::uint8_t>(((e.owner + 1) << 4) | 15);
  }
  if (!initialize_arena_routing_grid(visibility, routes_, grid_))
    throw std::invalid_argument("initial grid blocks routes");
  for (auto &bank : storage_.banks)
    bank.resize(c.capacity);
  reset_arena_entity_index_pool(storage_, c.capacity);
  for (const auto &e : c.initial_structures) {
    ArenaEntitySlot slot{};
    if (!initialize_arena_structure_entity(slot, e.type, e.owner, e.grid,
                                           *c.tuning[e.type], &random_))
      throw std::invalid_argument("constructor failed");
    for (unsigned i = 0; i < 4; ++i)
      scalar(slot, 0x20 + 4 * i, e.orientation[i]);
    if (!settle_arena_map_structure(slot, e.initial_upgrade, e.variant) ||
        !insert_arena_entity(storage_, e.grid, slot))
      throw std::invalid_argument("map import failed");
  }
}
ArenaConstructionContext
ArenaConstructionSimulation::context_for(ArenaConstructionRequest r) const {
  auto flat = r.grid_z * 24 + r.grid_x;
  ArenaConstructionContext c;
  c.visibility = grid_.cells[flat].visibility;
  c.cell_entry_count =
      std::max(storage_.cells[0][flat].size(), storage_.cells[1][flat].size());
  c.available_entities = storage_.free_indices.size();
  c.resource = config_.players[r.player].resource;
  c.resource_max = config_.players[r.player].resource_max;
  return c;
}
ArenaConstructionSubmit
ArenaConstructionSimulation::enqueue(ArenaConstructionRequest r) {
  if (r.player >= players_.size() || r.grid_x >= 24 || r.grid_z >= 24 ||
      !supported(r.opcode) || !config_.tuning[r.opcode])
    return ArenaConstructionSubmit::InvalidRequest;
  switch (validate_arena_construction(r, context_for(r), queue_)) {
  case ArenaConstructionError::InvalidRequest:
    return ArenaConstructionSubmit::InvalidRequest;
  case ArenaConstructionError::Occupied:
    return ArenaConstructionSubmit::Occupied;
  case ArenaConstructionError::CellFull:
    return ArenaConstructionSubmit::CellFull;
  case ArenaConstructionError::NoEntityCapacity:
    return ArenaConstructionSubmit::NoEntityCapacity;
  case ArenaConstructionError::Duplicate:
    return ArenaConstructionSubmit::Duplicate;
  case ArenaConstructionError::InsufficientResources:
    return ArenaConstructionSubmit::InsufficientResources;
  case ArenaConstructionError::None:
    break;
  }
  return queue_.push(r) ? ArenaConstructionSubmit::Queued
                        : ArenaConstructionSubmit::QueueFull;
}
ArenaConstructionSimulationStep ArenaConstructionSimulation::step() {
  if (queue_.empty())
    return {};
  ArenaConstructionSimulationStep s;
  auto preview = queue_;
  s.request = preview.pop();
  auto c = context_for(s.request);
  s.status = process_next_arena_construction(
      queue_, grid_, routes_, players_, c, [&](ArenaConstructionRequest r) {
        ArenaEntitySlot slot{};
        auto next_random = random_;
        if (!initialize_arena_structure_entity(
                slot, r.opcode, r.player, {r.grid_x, r.grid_z},
                *config_.tuning[r.opcode], &next_random))
          return false;
        s.entity_index =
            insert_arena_entity(storage_, {r.grid_x, r.grid_z}, slot);
        if (!s.entity_index)
          return false;
        random_ = next_random;
        return true;
      });
  config_.players[s.request.player].resource = c.resource;
  return s;
}
ArenaTimedSubmit ArenaConstructionSimulation::enqueue_event(std::size_t owner,
                                                            ArenaEvent e) {
  if (owner >= players_.size() || !std::isfinite(e.time) ||
      (e.selector < 9 && (e.grid_x >= 24 || e.grid_z >= 24)))
    return ArenaTimedSubmit::Invalid;
  auto &p = players_[owner];
  if (p.events.size() >= p.event_capacity)
    return ArenaTimedSubmit::Full;
  p.events.push(e);
  return ArenaTimedSubmit::Scheduled;
}
ArenaConstructionTick ArenaConstructionSimulation::advance(float clock) {
  if (!std::isfinite(clock) || clock < clock_)
    throw std::invalid_argument("clock must be finite and monotonic");
  clock_ = clock;
  ArenaConstructionTick tick;
  for (std::size_t owner = 0; owner < players_.size(); ++owner) {
    auto &p = players_[owner];
    auto &status = tick.players[owner];
    if (p.dispatch_blocked)
      continue;
    if (p.command_mode == 11) {
      if (p.events.empty() || !(p.events.next().time < clock))
        continue;
      auto e = p.events.next();
      if (e.selector != 0 || !supported(e.opcode) ||
          !config_.tuning[e.opcode]) {
        status = ArenaTimedStatus::Unsupported;
        continue;
      }
      auto dispatch = dispatch_arena_event(
          p, clock, random_, [&](std::uint8_t x, std::uint8_t z) {
            return resolve_arena_storage_target(
                storage_, grid_.cells[z * 24 + x].visibility,
                config_.event_exclusion_masks[owner], {x, z});
          });
      if (dispatch == ArenaDispatchResult::Retried) {
        status = ArenaTimedStatus::Retried;
        continue;
      }
      if (dispatch != ArenaDispatchResult::Pending)
        continue;
    }
    auto pending = p.pending;
    if (pending.selector != 0 || !supported(pending.opcode) ||
        !config_.tuning[pending.opcode]) {
      status = ArenaTimedStatus::Unsupported;
      continue;
    }
    auto submitted = enqueue({static_cast<std::uint8_t>(owner), pending.opcode,
                              pending.grid_x, pending.grid_z});
    if (submitted == ArenaConstructionSubmit::Queued) {
      take_arena_pending_command(p);
      status = ArenaTimedStatus::Submitted;
    } else if (submitted == ArenaConstructionSubmit::InvalidRequest)
      status = ArenaTimedStatus::Unsupported;
    else if (p.events.size() >= p.event_capacity)
      status = ArenaTimedStatus::Backpressure;
    else {
      schedule_arena_event(
          p, {clock, pending.grid_x, pending.grid_z, 0, pending.opcode}, clock,
          random_);
      take_arena_pending_command(p);
      status = ArenaTimedStatus::Retried;
    }
  }
  tick.construction = step();
  return tick;
}
} // namespace comet::decomp
