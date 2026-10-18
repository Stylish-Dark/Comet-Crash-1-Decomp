#include "comet/arena_construction_config.hpp"
#include "comet/native_construction_scene.hpp"
#include "comet/native_mission_playback.hpp"
#include <cassert>
#include <memory>
#include <sstream>
#include <functional>
#include <stdexcept>
using namespace comet::decomp;
static std::unique_ptr<NativeConstructionScene> scene(bool enabled=true, float funds=400,
    std::function<void(ArenaConstructionSimulationConfig&)> modify={},
    std::function<void(ArenaState&)> map_modify={}) {
  std::ostringstream text;
  text << "world 16 4\nplayer 1 1 0 400 400 0\nplayer 6 1 1 " << funds << " 400 0\n"
       << "tuning 26 100 50 10 20 7.5\nproduction 1\ntransport 1\n"
       << "unit-base 0 1\nunit-base 1 1\nunit-quota 1 0 2\n"
       << "base-counter 0 2\nbase-counter 1 2\n";
  if(enabled) text << "opponent 1\n";
  text << "run\n";
  std::istringstream input(text.str());std::size_t line=0;
  auto config=read_arena_construction_config(input,line);
  if(modify)modify(config);
  ArenaState arena;arena.players.resize(config.players.size());
  for(unsigned i=0;i<config.players.size();++i)arena.players[i].start_grid=config.players[i].route.base;
  if(map_modify)map_modify(arena);
  return std::make_unique<NativeConstructionScene>(arena,config);
}
int main() {
  auto active=scene();NativeMissionPlayback match(*active);
  bool built=false,produced=false,deployed=false,hit=false;
  for(unsigned i=0;i<600 && !match.finished();++i) {
    auto tick=active->advance(active->simulation().clock()+.1f);
    built|=tick.construction.entity_index.has_value();
    produced|=!tick.production_reservations.empty();
    deployed|=!tick.ground_deployments.empty() && tick.ground_deployments.back().result.entity_index.has_value();
    hit|=!tick.base_impacts.empty();
    const auto &quota=active->simulation().unit_quotas()[1];
    assert(unsigned(quota.active)+quota.pending<=quota.capacity);
  }
  assert(built && produced && deployed && hit);
  assert(active->simulation().accounts()[1].resource==340);
  assert(active->simulation().base_counter(0)==0 && match.finished());
  const auto banks=active->simulation().storage().banks;
  match.advance(1);assert(active->simulation().storage().banks==banks);
  match.restart();assert(active->simulation().clock()==0 && !match.finished());
  for(unsigned i=0;i<100;++i)match.advance(.1f);
  const auto checkpoint=active->simulation().storage().banks;
  match.restart();for(unsigned i=0;i<100;++i)match.advance(.1f);
  assert(active->simulation().storage().banks==checkpoint);
  auto disabled=scene(false),poor=scene(true,59);
  for(unsigned i=0;i<100;++i){disabled->advance(float(i+1)*.1f);poor->advance(float(i+1)*.1f);}
  assert(disabled->arena().entities.empty() && poor->arena().entities.empty());
  assert(poor->simulation().accounts()[1].resource==59);
  // Occupied nearest build cell: choose another cell and charge only once.
  auto occupied=scene();
  assert(occupied->enqueue({0,26,6,0})==ArenaConstructionSubmit::Queued);
  occupied->step();occupied->advance(.1f);
  assert(occupied->arena().entities.size()==2);
  assert(occupied->simulation().accounts()[1].resource==340);
  assert(occupied->arena().entities[1].grid!=ArenaGridPosition(6,0));
  // Allocation pressure never grants a free structure or changes the balance.
  auto full=scene(true,400,[](auto &c){c.capacity=0;});
  full->advance(.1f);assert(full->arena().entities.empty());
  assert(full->simulation().accounts()[1].resource==400);
  // Friendly-only and eliminated encounters never start production or deployment.
  for(unsigned mode=0;mode<2;++mode) {
    auto stopped=scene(true,400,[&](auto &c){
      if(mode==0)c.players[0].route.team=1;
      else c.base_counters[1]=0;
      c.unit_quotas[1].pending=1;c.unit_quotas[1].by_type[0]=1;
    });
    stopped->advance(2);
    assert(stopped->arena().entities.empty());
    assert(stopped->simulation().ground_deployment()[1]->strength==0);
  }
  // Clock rejection must not advance policy state or allocate/debit anything.
  auto atomic=scene(),reference=scene();bool rejected=false;
  try{atomic->advance(11);}catch(const std::invalid_argument&){rejected=true;}
  assert(rejected && atomic->simulation().clock()==0);
  atomic->advance(.1f);reference->advance(.1f);
  assert(atomic->simulation().storage().banks==reference->simulation().storage().banks);
  assert(atomic->simulation().accounts()[1].resource==reference->simulation().accounts()[1].resource);
  // Constructor validates programmatic configs as strictly as textual ones.
  for(unsigned mode=0;mode<7;++mode) {
    bool invalid=false;
    try{auto bad=scene(true,400,[&](auto &c){
      if(mode==0)c.production_enabled=false;
      if(mode==1)c.ground_transport_enabled=false;
      if(mode==2)c.unit_base_available[1]=false;
      if(mode==3)c.unit_quotas[1].capacity=0;
      if(mode==4)c.tuning[26].reset();
      if(mode==5)c.players[1].route.inactive=true;
      if(mode==6)c.ground_deployment[1]=ArenaGroundDeploymentControl{40,1};
    });}catch(const std::invalid_argument&){invalid=true;}
    assert(invalid);
  }

  // An already-built factory must also stop reserving units after elimination.
  auto defeated_factory=scene(true,400,[](auto &c){c.base_counters[1]=0;});
  assert(defeated_factory->enqueue({1,26,6,0})==ArenaConstructionSubmit::Queued);
  defeated_factory->step();
  for(unsigned i=1;i<=100;++i)defeated_factory->advance(float(i)*.1f);
  assert(defeated_factory->simulation().unit_quotas()[1].pending==0);
  assert(defeated_factory->simulation().unit_quotas()[1].active==0);

  // In a three-team encounter, new attacks must select a surviving enemy.
  for(unsigned mode=0;mode<3;++mode) {
  auto three=scene(true,400,[&](auto &c){
    c.players.push_back({{{10,1},2,false},400,400});
    c.unit_base_available[2]=true;c.base_counters[2]=10;
    if(mode==0)c.base_counters[0]=0;
    else if(mode==1)c.players[0].route.inactive=true;
    else c.players[0].route.team=1;
    c.unit_target_overrides[1]=0;
  });
  bool attacked_survivor=false;
  for(unsigned i=1;i<=150;++i) {
    auto tick=three->advance(float(i)*.1f);
    for(const auto &unit:three->simulation().storage().banks[0])
      if(unit[15]==40 && (unit[0x33]>>5)==1)assert(unit[0x7e]==2);
    for(const auto &attempt:tick.ground_deployments)if(attempt.result.entity_index) {
      const auto &unit=three->simulation().storage().banks[0][*attempt.result.entity_index];
      assert(unit[0x7e]==2);attacked_survivor=true;
    }
  }
  assert(attacked_survivor);
  }

  // The nearest empty cell closes the only corridor and must be rejected.
  // A later decision advances to a different candidate, without a second charge.
  auto corridor=scene(true,400,{},[](auto &a){
    for(unsigned z=0;z<24;++z)if(z!=1)a.cells[z*24+5].visibility=0x1f;
    a.cells[6].visibility=0x1f;
  });
  const auto rejected_build=corridor->advance(.1f);
  assert(rejected_build.construction.status==ArenaConstructionStep::RouteRejected);
  assert(corridor->arena().entities.empty());
  assert(corridor->simulation().accounts()[1].resource==400);
  NativeMissionPlayback route_checkpoint(*corridor);
  const auto accepted_build=corridor->advance(1.2f);
  assert(accepted_build.construction.entity_index);
  assert(corridor->simulation().accounts()[1].resource==340);
  const auto accepted_banks=corridor->simulation().storage().banks;
  route_checkpoint.restart();corridor->advance(1.2f);
  assert(corridor->simulation().storage().banks==accepted_banks);

}
