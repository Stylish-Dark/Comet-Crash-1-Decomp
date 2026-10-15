#include "comet/native_mission_playback.hpp"
#include <cassert>
#include <memory>
using namespace comet::decomp;

int main() {
  ArenaConstructionSimulationConfig c;
  c.capacity=4; c.seed=4; c.ground_transport_enabled=true;
  c.players={{{{0,0},0,false},100,100},{{{4,4},1,false},100,100}};
  c.unit_base_available[0]=c.unit_base_available[1]=true;
  c.base_counters[0]=2; c.base_counters[1]=1;
  c.unit_quotas[0].capacity=2; c.unit_quotas[0].pending=2; c.unit_quotas[0].by_type[0]=2;
  c.tuning[20]=ArenaStructureTableValues{100,50,10,20,7.5f};
  ArenaState arena; arena.players.resize(2);
  arena.players[0].start_grid=ArenaGridPosition{0,0};
  arena.players[1].start_grid=ArenaGridPosition{4,4};
  auto scene=std::make_unique<NativeConstructionScene>(arena,c);
  // Snapshot includes launch-time queued construction and a unit already in flight.
  assert(scene->spawn_reserved_ground_unit(0,40,{4,4}).entity_index==0);
  assert(scene->enqueue({0,20,8,8})==ArenaConstructionSubmit::Queued);
  auto initial=std::make_unique<NativeConstructionScene>(*scene);
  NativeMissionPlayback playback(*scene);
  assert(!playback.finished() && !playback.playing());
  playback.toggle_playing(); assert(playback.playing());
  assert(scene->set_ground_deployment(0,{40,1}));
  playback.advance(.01f); // Actual base hit plus construction commit, not fabricated counters.
  assert(playback.finished() && !playback.playing());
  assert(evaluate_native_mission(scene->simulation(),0)==NativeMissionOutcome::Victory);
  assert(evaluate_native_mission(scene->simulation(),1)==NativeMissionOutcome::Defeat);
  assert(scene->simulation().ground_deployment()[0]->strength==0);
  const auto stopped=scene->simulation().clock();
  const auto banks=scene->simulation().storage().banks;
  playback.toggle_playing(); assert(!playback.playing());
  playback.advance(1); assert(scene->simulation().clock()==stopped);
  assert(scene->simulation().storage().banks==banks);
  playback.restart();
  assert(!playback.finished() && !playback.playing());
  assert(scene->simulation().clock()==initial->simulation().clock());
  assert(scene->simulation().pending()==1 && scene->simulation().accounts()[0].resource==100);
  assert(scene->simulation().base_counter(1)==1);
  assert(scene->simulation().storage().banks==initial->simulation().storage().banks);
  assert(scene->simulation().storage().cells==initial->simulation().storage().cells);
  assert(scene->simulation().storage().free_indices==initial->simulation().storage().free_indices);
  assert(scene->simulation().unit_quotas()[0].pending==1);
  assert(scene->arena().entities.size()==initial->arena().entities.size());
  assert(!scene->arena().cells[8*24+8].entity_index);
  // Restored RNG must produce the same constructor/next-deployment bytes.
  scene->advance(.01f); initial->advance(.01f);
  assert(scene->simulation().storage().banks==initial->simulation().storage().banks);
  assert(scene->spawn_reserved_ground_unit(0,40,{0,0}).entity_index==
         initial->spawn_reserved_ground_unit(0,40,{0,0}).entity_index);
  assert(scene->simulation().storage().banks==initial->simulation().storage().banks);
  playback.restart(); playback.restart();
  assert(scene->simulation().pending()==1 && scene->simulation().clock()==0);

  // Eliminating one side of a three-team match must not freeze the other two.
  c.players.push_back({{{6,6},2,false},100,100}); c.base_counters[0]=0; c.base_counters[2]=2;
  arena.players.push_back({}); arena.players[2].start_grid=ArenaGridPosition{6,6};
  auto multiple=std::make_unique<NativeConstructionScene>(arena,c);
  NativeMissionPlayback ongoing(*multiple);
  assert(evaluate_native_mission(multiple->simulation(),0)==NativeMissionOutcome::Defeat);
  assert(!ongoing.finished()); ongoing.advance(.1f); assert(multiple->simulation().clock()==.1f);
  c.base_counters[1]=0;
  auto terminal=std::make_unique<NativeConstructionScene>(arena,c);
  NativeMissionPlayback won(*terminal); assert(won.finished()); won.advance(1);
  assert(terminal->simulation().clock()==0);
  c.base_counters[2]=0;
  auto drawn=std::make_unique<NativeConstructionScene>(arena,c);
  NativeMissionPlayback draw(*drawn); assert(draw.finished());
  c.base_counters[2].reset();
  auto incomplete=std::make_unique<NativeConstructionScene>(arena,c);
  NativeMissionPlayback partial(*incomplete); assert(!partial.finished());
  partial.advance(.1f); assert(incomplete->simulation().clock()==.1f);
}
