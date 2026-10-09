#include "comet/arena_state.hpp"
#include "comet/arena_assets.hpp"
#include <bit>
#include <cmath>
#include <stdexcept>
using namespace comet::decomp;
void check(bool value) {
  if (!value)
    throw std::runtime_error("arena state check failed");
}
void be_float(auto &bytes, std::size_t offset, float value) {
  auto bits = std::bit_cast<std::uint32_t>(value);
  for (int i = 0; i < 4; ++i)
    bytes[offset + i] = static_cast<std::uint8_t>(bits >> (24 - i * 8));
}
LevelMapPrimaryRecord primary(std::uint8_t type, std::uint8_t x, std::uint8_t z,
                              bool alternate = false) {
  LevelMapPrimaryRecord r;
  r.raw[0] = type;
  r.raw[1] = 1;
  r.raw[2] = x;
  r.raw[3] = z;
  r.raw[20] = alternate;
  be_float(r.raw, 16, 1);
  return r;
}
LevelMapSecondaryRecord event(float time, std::uint8_t x, std::uint8_t z,
                              std::uint8_t selector, std::uint8_t opcode,
                              bool alternate = false) {
  LevelMapSecondaryRecord r;
  be_float(r.raw, 0, time);
  r.raw[4] = x;
  r.raw[5] = z;
  r.raw[6] = selector;
  r.raw[7] = opcode;
  r.raw[8] = alternate;
  return r;
}
int main() {
  const std::array<ArenaCellEntry, 4> entries{
      {{5, 0x0c}, {6, 0x10}, {7, 0x20}, {8, 0x40}}};
  check(resolve_arena_cell_target(0x1f, 0, entries) == 7);
  check(resolve_arena_cell_target(0x1f, 2, entries) == 8);
  check(resolve_arena_cell_target(0x1f, 1, entries) == -1);
  check(resolve_arena_cell_target(0x0f, 0, entries) == -1);
  check(resolve_arena_cell_target(0x9f, 0, entries) == -1);
  check(resolve_arena_cell_target(0x1f, 6, entries) == -1);
  ArenaRandom random(1);
  constexpr std::array<std::uint32_t, 8> expected_random{
      330177771, 135115861, 3885783,   761386273,
      598057187, 740356877, 448971087, 295840345};
  for (auto value : expected_random)
    check(random.next() == value);
  random.seed(1);
  check(random.next() == expected_random[0]);
  ArenaPlayer timed;
  ArenaRandom timing_random(1);
  check(schedule_arena_event(timed, {0, 2, 3, 0, 26}, 10, timing_random, 2) ==
        3);
  auto e0 = timed.events.pop();
  check(e0.time > 15.53f && e0.time < 15.54f && e0.selector == 0);
  auto e1 = timed.events.pop();
  check(e1.time > e0.time && e1.selector == 1);
  check(timed.events.pop().time > e1.time);
  timed.event_capacity = 0;
  random.seed(1);
  check(schedule_arena_event(timed, {0, 2, 3, 3, 21}, 0, random) == 0);
  check(random.next() == expected_random[1]);
  timed.event_capacity = 4;
  random.seed(1);
  check(schedule_arena_event(timed, {0, 2, 3, 3, 21}, 0, random) == 1);
  check(timed.events.next().time < 0.2f);
  ArenaPlayer dispatch;
  dispatch.events.push({1, 2, 3, 0, 26});
  auto empty_target = [](std::uint8_t, std::uint8_t) {
    return ArenaEventTarget{};
  };
  check(dispatch_arena_event(dispatch, 1, random, empty_target) ==
        ArenaDispatchResult::Idle);
  check(dispatch_arena_event(dispatch, 2, random, empty_target) ==
        ArenaDispatchResult::Pending);
  check(dispatch.pending.entity_index == -1 && dispatch.pending.selector == 0 &&
        dispatch.pending.grid_x == 2 && dispatch.pending.source_flag == 1);
  auto pending = take_arena_pending_command(dispatch);
  check(pending && pending->opcode == 26);
  check(dispatch.command_mode == 11);
  check(!take_arena_pending_command(dispatch));
  dispatch.events.push({1, 2, 3, 0, 26});
  random.seed(1);
  auto occupied = [](std::uint8_t, std::uint8_t) {
    return ArenaEventTarget{7, 26};
  };
  check(dispatch_arena_event(dispatch, 2, random, occupied) ==
        ArenaDispatchResult::Retried);
  check(dispatch.events.size() == 1 && dispatch.events.next().time > 2);
  dispatch.events.pop();
  dispatch.events.push({1, 2, 3, 1, 26});
  check(dispatch_arena_event(dispatch, 2, random, occupied) ==
        ArenaDispatchResult::Pending);
  check(dispatch.pending.entity_index == 7);
  take_arena_pending_command(dispatch);
  dispatch.events.push({1, 255, 254, 9, 0});
  check(dispatch_arena_event(dispatch, 2, random, {}) ==
        ArenaDispatchResult::Pending);
  check(dispatch.pending.entity_index == -1);
  take_arena_pending_command(dispatch);
  auto ice = arena_environment_asset_manifest(0);
  check(ice.size() == 3);
  check(ice[0].original_slot_index == 34);
  check(ice[0].geometry_scale == 1);
  check(ice[1].path == "models/care/cometIce/cometIce.obj");
  check(ice[2].original_loader_options == 0x414);
  auto rock = arena_environment_asset_manifest(1);
  check(rock[0].geometry_scale == 1.8f);
  check(rock[1].original_root_offset == 0x2D4290);
  check(arena_environment_asset_manifest(2)[0].geometry_scale == 0.95f);
  check(arena_environment_asset_manifest(3)[1].path ==
        "models/care/cometLava/lavaComet.obj");
  check(arena_environment_asset_manifest(4).empty());
  LevelMapData map;
  map.has_extent_record = true;
  map.arena_extent = 16;
  map.header.raw[8] = 1;
  map.primary_records = {primary(11, 16, 16), primary(10, 14, 1),
                         primary(9, 2, 3), primary(21, 4, 5),
                         primary(29, 1, 1, true)};
  map.secondary_records = {event(2, 4, 5, 0, 26), event(1, 1, 2, 9, 0, true),
                           event(2, 3, 4, 1, 21), event(0, 0, 0, 0, 29)};
  ArenaState arena;
  check(bool(initialize_arena_state(map, {}, arena)));
  check(arena.extent == 16);
  check(arena.theme == 1);
  check(arena.entities.size() == 3);
  check(arena.entities[0].model_slot == 34);
  check(arena.entities[0].position.x == 2.5f);
  check(arena.entities[0].position.y == -0.2f);
  check(arena.entities[0].position.z == 3.5f);
  check(arena.background_position.x == 4 && arena.background_position.z == 4);
  check(arena.background_orientation[1] > 0.7f &&
        arena.background_orientation[3] > 0.7f);
  check(arena.entities[1].model_slot == 11);
  check(arena.entities[1].owner == 0);
  check(arena.entities[2].model_slot == 21);
  check(arena.entities[2].owner == 1);
  check(arena.players[0].start_grid == ArenaGridPosition{14, 1});
  check(arena.cell(4, 5)->entity_index == 1);
  check(arena.cell(2, 3)->visibility == 0x9f);
  check(arena.players[0].events.size() == 2);
  check(arena.players[1].events.size() == 1);
  check(!pop_due_arena_event(arena.players[1], 1));
  auto first = pop_due_arena_event(arena.players[1], 1.01f);
  check(first.has_value());
  check(first->selector == 9 && first->opcode == 0 && first->grid_x == 1);
  arena.players[0].dispatch_blocked = true;
  check(!pop_due_arena_event(arena.players[0], 3));
  arena.players[0].dispatch_blocked = false;
  arena.players[0].command_mode = 3;
  check(!pop_due_arena_event(arena.players[0], 3));
  arena.players[0].command_mode = 11;
  check(!pop_due_arena_event(arena.players[0], 2));
  auto a = pop_due_arena_event(arena.players[0], 3);
  check(a && a->opcode == 26);
  check(arena.players[0].events.size() == 1);
  auto b = pop_due_arena_event(arena.players[0], 3);
  check(b && b->opcode == 21);
  auto old = arena.entities.size();
  map.primary_records.push_back(primary(21, 24, 0));
  check(!initialize_arena_state(map, {}, arena));
  check(arena.entities.size() == old);
  map.primary_records.pop_back();
  map.primary_records.push_back(primary(20, 0, 0, true));
  ArenaLoadConfig one;
  one.secondary_player = -1;
  check(bool(initialize_arena_state(map, one, arena)));
  check(arena.entities.size() == 2);
  check(arena.players[1].events.empty());
  map.primary_records.back().raw[2] = 25;
  check(bool(initialize_arena_state(map, one,
                                    arena))); // skipped route is never indexed
  map.primary_records.push_back(primary(10, 1, 2));
  map.primary_records.back().raw[1] = 0;
  check(!initialize_arena_state(map, {}, arena));
  ArenaEventQueue heap;
  for (int i = 0; i < 65; ++i)
    heap.push({float(i % 7), std::uint8_t(i), 0, 0, 20});
  float last = -1;
  while (!heap.empty()) {
    auto e = heap.pop();
    check(e.time >= last);
    last = e.time;
  }
  check(heap.empty());
}
