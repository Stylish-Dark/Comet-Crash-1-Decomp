#include "comet/arena_state.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace comet::decomp {
namespace {
float read_float(const std::uint8_t *p) {
  std::uint32_t v = std::uint32_t(p[0]) << 24 | std::uint32_t(p[1]) << 16 |
                    std::uint32_t(p[2]) << 8 | p[3];
  return std::bit_cast<float>(v);
}
ArenaStateResult fail(std::string detail) { return {false, std::move(detail)}; }
} // namespace
void ArenaEventQueue::push(ArenaEvent event) {
  if (!std::isfinite(event.time))
    throw std::invalid_argument("nonfinite arena event time");
  heap_.push_back(event);
  auto index = heap_.size() - 1;
  while (index) {
    auto parent = (index - 1) / 2;
    if (!(heap_[index].time < heap_[parent].time))
      break;
    std::swap(heap_[index], heap_[parent]);
    index = parent;
  }
}
const ArenaEvent &ArenaEventQueue::next() const {
  if (empty())
    throw std::out_of_range("empty arena event heap");
  return heap_.front();
}
ArenaEvent ArenaEventQueue::pop() {
  auto result = next();
  heap_.front() = heap_.back();
  heap_.pop_back();
  std::size_t parent = 0;
  while (parent * 2 + 1 < heap_.size()) {
    auto child = parent * 2 + 1;
    auto right = child + 1;
    // Original selects the right child when both children have equal time.
    if (right < heap_.size() && heap_[right].time <= heap_[child].time)
      child = right;
    if (heap_[parent].time <= heap_[child].time)
      break;
    std::swap(heap_[parent], heap_[child]);
    parent = child;
  }
  return result;
}
std::uint32_t ArenaRandom::step() {
  seed_ = seed_ * 1664525u + 1013904223u;
  return seed_;
}
std::uint32_t ArenaRandom::next() {
  if (!initialized_) {
    for (int i = 0; i < 8; ++i)
      step();
    for (auto &value : shuffle_)
      value = step();
    selector_ = shuffle_.back();
    initialized_ = true;
  }
  const auto value = step();
  const auto index = selector_ & 31u;
  selector_ = shuffle_[index];
  shuffle_[index] = value;
  return selector_ & 0x3fffffffu;
}
std::size_t schedule_arena_event(ArenaPlayer &player, ArenaEvent event,
                                 float clock, ArenaRandom &random,
                                 std::int32_t extra_count) {
  if (!std::isfinite(clock))
    throw std::invalid_argument("nonfinite arena clock");
  std::size_t inserted = 0;
  auto insert = [&] {
    if (player.events.size() < player.event_capacity) {
      player.events.push(event);
      ++inserted;
    }
  };
  // PPC frsp rounds the integer conversion before multiplication; fmadds
  // performs one final single-precision rounding of the multiply/add.
  const float unit = static_cast<float>(random.next()) * 0x1p-30f;
  const float delay =
      event.selector == 3 ? unit * 0.5f : std::fma(unit, 5.0f, 4.0f);
  event.time = clock + delay;
  insert();
  for (std::int32_t i = 0; i < extra_count; ++i) {
    const float next_unit = static_cast<float>(random.next()) * 0x1p-30f;
    event.time += std::fma(next_unit, 5.0f, 4.0f);
    event.selector = 1;
    insert();
  }
  return inserted;
}
ArenaDispatchResult dispatch_arena_event(ArenaPlayer &player, float clock,
                                         ArenaRandom &random,
                                         const ArenaTargetResolver &resolve) {
  const auto event = pop_due_arena_event(player, clock);
  if (!event)
    return ArenaDispatchResult::Idle;
  ArenaEventTarget target;
  if (event->selector != 9 && event->selector != 10) {
    target = resolve(event->grid_x, event->grid_z);
    const bool ready = event->selector == 0 ? target.entity_index == -1
                                            : target.entity_index != -1 &&
                                                  target.type == event->opcode;
    if (!ready) {
      schedule_arena_event(player, *event, clock, random);
      return ArenaDispatchResult::Retried;
    }
  }
  player.pending = {target.entity_index, event->selector, event->opcode,
                    event->grid_x,       event->grid_z,   1};
  player.command_mode = event->selector;
  return ArenaDispatchResult::Pending;
}
std::int32_t
resolve_arena_cell_target(std::uint8_t visibility, std::uint32_t mask,
                          std::span<const ArenaCellEntry> entries) {
  const auto owner = static_cast<std::uint32_t>(visibility >> 4) - 1u;
  if (owner > 3 || ((mask >> owner) & 1u))
    return -1;
  for (const auto &entry : entries) {
    const auto kind = entry.flags & 0x1c;
    if (kind != 0x0c && kind != 0x10 &&
        ((mask >> (entry.flags >> 5)) & 1u) == 0)
      return entry.entity_index;
  }
  return -1;
}
std::optional<ArenaPendingCommand>
take_arena_pending_command(ArenaPlayer &player) {
  if (player.command_mode == 11)
    return {};
  auto pending = player.pending;
  player.command_mode = 11;
  player.pending.selector = 11;
  return pending;
}
std::optional<ArenaEvent> pop_due_arena_event(ArenaPlayer &player,
                                              float clock) {
  if (!std::isfinite(clock) || player.dispatch_blocked ||
      player.command_mode != 11 || player.events.empty() ||
      !(player.events.next().time < clock))
    return {};
  return player.events.pop();
}
const ArenaCell *ArenaState::cell(std::uint32_t x, std::uint32_t z) const {
  if (x >= kArenaGridStride || z >= kArenaGridStride)
    return nullptr;
  return &cells[z * kArenaGridStride + x];
}
std::optional<std::uint8_t> initial_model_slot_for_record(std::uint8_t type) {
  switch (type) {
  case 9:
    return 34;
  case 20:
    return 17;
  case 21:
    return 11;
  case 22:
    return 16;
  case 23:
    return 14;
  case 24:
    return 18;
  case 25:
    return 35;
  case 26:
    return 24;
  case 28:
    return 25;
  case 29:
    return 21;
  default:
    return {};
  }
}
ArenaStateResult initialize_arena_state(const LevelMapData &map,
                                        const ArenaLoadConfig &config,
                                        ArenaState &output) {
  if (!config.player_count || config.player_count > 4 ||
      config.primary_player >= config.player_count ||
      config.secondary_player < -1 ||
      config.secondary_player >= static_cast<int>(config.player_count))
    return fail("invalid arena player routing");
  ArenaState next;
  next.players.resize(config.player_count);
  next.theme = map.header.raw[8];
  bool has_extent = false;
  for (const auto &record : map.secondary_records) {
    if (!keep_level_map_secondary_record(record))
      continue;
    const int owner = record.raw[8] ? config.secondary_player
                                    : static_cast<int>(config.primary_player);
    if (owner < 0)
      continue;
    ArenaEvent event{record.order_value(), record.raw[4], record.raw[5],
                     record.selector(), record.opcode()};
    if (!std::isfinite(event.time))
      return fail("nonfinite scheduled arena event");
    if (event.selector < 9 &&
        (event.grid_x >= kArenaGridStride || event.grid_z >= kArenaGridStride))
      return fail("scheduled event outside 24x24 grid");
    next.players[owner].events.push(event);
  }
  for (std::size_t index = 0; index < map.primary_records.size(); ++index) {
    const auto &record = map.primary_records[index];
    const auto &raw = record.raw;
    if (record.type() == 11) {
      next.extent = std::max(raw[2], raw[3]);
      has_extent = true;
      continue;
    }
    if (record.type() == 10) {
      if (raw[1] < 1 || raw[1] > config.player_count ||
          raw[2] >= kArenaGridStride || raw[3] >= kArenaGridStride)
        return fail("invalid player start record");
      next.players[raw[1] - 1].start_grid = ArenaGridPosition{raw[2], raw[3]};
      continue;
    }
    const int owner = raw[20] ? config.secondary_player
                              : static_cast<int>(config.primary_player);
    if (owner < 0)
      continue;
    const auto slot = initial_model_slot_for_record(record.type());
    if (!slot) {
      next.unsupported_records.push_back(index);
      continue;
    }
    if (raw[2] >= kArenaGridStride || raw[3] >= kArenaGridStride)
      return fail("entity record outside 24x24 grid");
    ArenaEntity entity;
    entity.type = record.type();
    entity.model_slot = *slot;
    entity.initial_upgrade = raw[1];
    entity.variant = raw[21];
    entity.owner = record.type() == 9 ? 0 : owner;
    entity.grid = {raw[2], raw[3]};
    entity.position = {raw[2] + 0.5f, record.type() == 9 ? -0.2f : 0.0f,
                       raw[3] + 0.5f};
    entity.source_record = index;
    for (std::size_t i = 0; i < 4; ++i) {
      entity.orientation[i] = read_float(raw.data() + 4 + i * 4);
      if (!std::isfinite(entity.orientation[i]))
        return fail("nonfinite entity orientation");
    }
    auto &cell = next.cells[raw[3] * kArenaGridStride + raw[2]];
    cell.entity_index = next.entities.size();
    cell.visibility =
        record.type() == 9
            ? 0x9f
            : static_cast<std::uint8_t>(((owner + 1) << 4) |
                                        (record.type() == 29 ? 0 : 15));
    if (record.type() == 25) {
      cell.visibility = 0;
      cell.visibility_recovered = false;
    }
    next.entities.push_back(entity);
  }
  if (!has_extent)
    next.extent = 24;
  if (!next.extent || next.extent > kArenaGridStride)
    return fail("arena extent outside native grid");
  // 0xEB438..0xEB680: global subroot is root+0xA00 (0xD51A8).
  if (next.theme < 4) {
    constexpr std::array<std::uint32_t, 4> last_shifted_level{3, 10, 17, 24};
    if (config.level_id <= last_shifted_level[next.theme])
      next.background_position = {static_cast<float>(next.extent) - 12, -0.2f,
                                  static_cast<float>(next.extent) - 12};
    const float half_angle = static_cast<float>(config.level_id % 4 + 1) *
                             1.5707963705062866f * 0.5f;
    next.background_orientation = {0, std::sin(half_angle), 0,
                                   std::cos(half_angle)};
  }
  output = std::move(next);
  return {};
}
} // namespace comet::decomp
