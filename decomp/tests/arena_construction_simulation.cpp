#include "comet/arena_construction_config.hpp"
#include "comet/native_construction_scene.hpp"
#include <cassert>
#include <cmath>
#include <sstream>
#include <stdexcept>
using namespace comet::decomp;
int main() {
  ArenaConstructionSimulationConfig c;
  c.capacity = 16;
  c.players = {{{{0, 0}, 0, false}, 400, 400}};
  for (unsigned op : {20, 21, 22, 23, 24, 25, 26, 28})
    c.tuning[op] = ArenaStructureTableValues{100, 50, 10, 20, 7.5f};
  ArenaConstructionSimulation s(c);
  assert(s.enqueue({0, 20, 2, 2}) == ArenaConstructionSubmit::Queued);
  assert(s.enqueue({0, 20, 2, 2}) == ArenaConstructionSubmit::Duplicate);
  auto result = s.step();
  assert(result.status == ArenaConstructionStep::Constructed &&
         result.entity_index == 0);
  assert(s.accounts()[0].resource == 400 - *arena_construction_cost(20));
  assert(s.storage().banks[0][0] == s.storage().banks[1][0]);
  assert(s.enqueue({0, 20, 2, 2}) == ArenaConstructionSubmit::Occupied);
  assert(s.enqueue_event(0, {2, 3, 3, 0, 21}) == ArenaTimedSubmit::Scheduled);
  assert(s.advance(2).construction.status == ArenaConstructionStep::Idle);
  auto tick = s.advance(3);
  assert(tick.players[0] == ArenaTimedStatus::Submitted &&
         tick.construction.status == ArenaConstructionStep::Constructed);
  auto clock = s.clock();
  try {
    s.advance(2);
    assert(false);
  } catch (const std::invalid_argument &) {
  }
  assert(s.clock() == clock);
  assert(s.enqueue_event(0, {4, 4, 4, 1, 20}) == ArenaTimedSubmit::Scheduled);
  assert(s.advance(5).players[0] == ArenaTimedStatus::Unsupported);
  assert(s.players()[0].events.size() == 1);
  c.capacity = 0;
  ArenaConstructionSimulation empty(c);
  assert(empty.enqueue({0, 20, 1, 1}) ==
         ArenaConstructionSubmit::NoEntityCapacity);
  c.capacity = 16;
  ArenaState a;
  a.extent = 8;
  a.players.resize(1);
  a.players[0].start_grid = ArenaGridPosition{0, 0};
  ArenaEntity e;
  e.type = 20;
  e.model_slot = *initial_model_slot_for_record(20);
  e.initial_upgrade = 1;
  e.grid = {2, 2};
  e.position = {2.5f, 0, 2.5f};
  a.entities.push_back(e);
  a.cells[50].visibility = 31;
  NativeConstructionScene scene(a, c);
  assert(scene.imports().live == 1 &&
         scene.arena().entities[0].live_entity_index == 0);
  const auto &slot = scene.simulation().storage().banks[0][0];
  assert(!(slot[0x63] & 8));
  assert(scene.simulation().accounts()[0].resource == 400);
  assert(scene.enqueue({0, 21, 3, 3}) == ArenaConstructionSubmit::Queued);
  assert(scene.step().status == ArenaConstructionStep::Constructed);
  assert(scene.arena().entities.size() == 2 &&
         scene.arena().entities.back().live_entity_index == 1);
  assert(scene.enqueue({0, 20, 8, 0}) ==
         ArenaConstructionSubmit::InvalidRequest);
  ArenaState shared = a;
  ArenaEntity rock;
  rock.type = 9;
  rock.grid = {2, 2};
  rock.position = {2.5f, 0, 2.5f};
  shared.entities.insert(shared.entities.begin(), rock);
  shared.entities[1].type = 25;
  shared.entities[1].model_slot = *initial_model_slot_for_record(25);
  shared.entities[1].initial_upgrade = 5;
  shared.cells[50].entity_index = 1;
  shared.cells[50].visibility_recovered = false;
  NativeConstructionScene shared_scene(shared, c);
  assert(shared_scene.imports().live == 1 &&
         shared_scene.imports().display_only == 1);
  assert(shared_scene.arena().cells[50].visibility ==
         arena_gate_visibility(0, 0));
  assert(shared_scene.enqueue_event(0, {1, 8, 0, 0, 20}) ==
         ArenaTimedSubmit::Invalid);
  auto limited = c;
  limited.players[0].resource = 20;
  ArenaConstructionSimulation funded(limited);
  assert(funded.enqueue({0, 20, 1, 1}) == ArenaConstructionSubmit::Queued);
  assert(funded.enqueue({0, 20, 2, 2}) == ArenaConstructionSubmit::Queued);
  assert(funded.step().status == ArenaConstructionStep::Constructed);
  assert(funded.step().status == ArenaConstructionStep::ReleaseQueued);
  assert(funded.storage().free_indices.size() == 15 &&
         funded.accounts()[0].resource == 0);
  assert(funded.step().status == ArenaConstructionStep::Released);
  assert(funded.grid().cells[50].visibility == 0);
  auto outside = a;
  outside.players[0].events.push({1, 8, 0, 0, 20});
  try {
    NativeConstructionScene invalid(outside, c);
    assert(false);
  } catch (const std::invalid_argument &) {
  }
  std::istringstream valid(
      "world 4 1\nplayer 0 0 0 400 400 0\ntuning 20 100 50 10 20 7.5\nrun\n");
  std::size_t line = 0;
  assert(read_arena_construction_config(valid, line).players.size() == 1 &&
         line == 4);
  for (auto text :
       {"world -1 1\nrun\n", "world 4 1\nplayer 0 0 0 nan 400 0\nrun\n",
        "world 4 1\nplayer 0 0 0 400 400 0\ntuning 20 1 0 0 0 0\nrun\n"}) {
    std::istringstream invalid(text);
    line = 0;
    try {
      read_arena_construction_config(invalid, line);
      assert(false);
    } catch (const std::invalid_argument &) {
    }
  }
}
