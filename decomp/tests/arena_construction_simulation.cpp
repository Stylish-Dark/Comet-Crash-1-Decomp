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
  a.cells[50].entity_index = 0;
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
  assert(scene.arena().entities[0].construction_ready);
  assert(scene.arena().entities[0].construction_progress == 1.0f);
  assert(!scene.arena().entities[1].construction_ready);
  scene.advance(1);
  assert(*scene.arena().entities[1].construction_progress > .49f);
  assert(!scene.arena().entities[1].construction_ready);
  scene.advance(2);
  assert(scene.arena().entities[1].construction_ready);
  assert(scene.arena().entities[1].construction_progress == 1.0f);
  assert(scene.enqueue({0, 20, 8, 0}) ==
         ArenaConstructionSubmit::InvalidRequest);
  ArenaState shared = a;
  ArenaEntity rock;
  rock.type = 9;
  rock.construction_progress = 1;
  rock.construction_ready = true;
  rock.production_mode = 4;
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
  assert(!shared_scene.arena().entities[0].construction_progress &&
         !shared_scene.arena().entities[0].construction_ready &&
         !shared_scene.arena().entities[0].production_mode);
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
  // Factory commands preserve the original dispatch gates and mutate both
  // banks.
  ArenaConstructionSimulation factory(c);
  assert(factory.enqueue({0, 26, 4, 4}) == ArenaConstructionSubmit::Queued);
  assert(factory.step().status == ArenaConstructionStep::Constructed);
  assert(factory.enqueue_event(0, {1, 4, 4, 8, 26}) ==
         ArenaTimedSubmit::Scheduled);
  assert(factory.advance(1).players[0] == ArenaTimedStatus::Idle);
  assert(factory.advance(2).players[0] == ArenaTimedStatus::Submitted);
  assert(factory.storage().banks[0][0][0xaa] == 4);
  assert(factory.select_factory(0, 8) == ArenaFactorySelection::Unchanged);
  assert(factory.storage().banks[0][0][0xaa] == 4);
  assert(factory.storage().banks[0][0] == factory.storage().banks[1][0]);
  assert(factory.players()[0].events.empty());
  assert(factory.accounts()[0].resource == 340);

  auto pending_config = c;
  pending_config.initial_players.resize(1);
  pending_config.initial_players[0].command_mode = 6;
  pending_config.initial_players[0].pending = {0, 6, 26, 4, 4, 1};
  ArenaConstructionSimulation pending_factory(pending_config);
  assert(pending_factory.enqueue({0, 26, 4, 4}) ==
         ArenaConstructionSubmit::Queued);
  pending_factory.step();
  auto applied = pending_factory.advance(1);
  assert(applied.players[0] == ArenaTimedStatus::Submitted);
  assert(applied.factory_selections == std::vector<std::uint16_t>{0});
  assert(pending_factory.storage().banks[0][0][0xaa] == 2);
  assert(pending_factory.players()[0].command_mode == 11);
  assert(scene.enqueue({0, 26, 5, 5}) == ArenaConstructionSubmit::Queued);
  scene.step();
  assert(scene.arena().entities.back().production_mode == 0);
  assert(scene.select_factory(0, {5, 5}, 7) == ArenaFactorySelection::Changed);
  assert(scene.arena().entities.back().production_mode == 3);
  assert(scene.select_factory(0, {8, 5}, 4) ==
         ArenaFactorySelection::InvalidRequest);
  assert(scene.select_factory(1, {5, 5}, 4) ==
         ArenaFactorySelection::InvalidRequest);
  assert(scene.select_factory(0, {2, 2}, 4) ==
         ArenaFactorySelection::Unsupported);
  assert(scene.select_factory(0, {7, 7}, 4) == ArenaFactorySelection::NoTarget);

  auto malformed = a;
  malformed.cells[50].entity_index = 999;
  try {
    NativeConstructionScene invalid(malformed, c);
    assert(false);
  } catch (const std::invalid_argument &) {
  }
  assert(!scene.simulation().factory_unit_template(65535));
  const auto inspected = scene.simulation().factory_unit_template(
      *scene.arena().entities.back().live_entity_index);
  assert(inspected && (*inspected)[15] == 43 && (*inspected)[0x6f] == 5);
  assert(inspected == scene.simulation().factory_unit_template(
                          *scene.arena().entities.back().live_entity_index));
  assert(!scene.simulation().factory_unit_template(0));
  const auto ground = scene.simulation().factory_ground_unit_template(
      *scene.arena().entities.back().live_entity_index);
  assert(ground && ground->binding == ArenaGroundUnitBinding::NoTarget &&
         ground->slot[0x7e] == 255);
  assert(ground->slot == *inspected && ground->constructor_result == 50);
  assert(!scene.simulation().factory_ground_unit_template(65535));
  auto production_config = c;
  production_config.production_enabled = true;
  production_config.unit_quotas[0].capacity = 2;
  ArenaConstructionSimulation producing(production_config);
  assert(producing.enqueue({0, 26, 4, 4}) == ArenaConstructionSubmit::Queued);
  producing.step();
  assert(producing.advance(2).production_reservations.empty());
  assert(producing.advance(4).production_reservations ==
         std::vector<std::uint16_t>{0});
  assert(producing.unit_quotas()[0].pending == 1 &&
         producing.unit_quotas()[0].by_type[0] == 1);
  assert(producing.accounts()[0].resource == 340 &&
         producing.storage().free_indices.size() == 15);
  assert(producing.storage().banks[0][0] == producing.storage().banks[1][0]);
}
