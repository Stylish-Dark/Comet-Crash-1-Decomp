#include "comet/arena_ground_deployment.hpp"
#include "comet/native_construction_scene.hpp"
#include <cassert>
#include <bit>
#include <cfenv>
#include <limits>
#include <memory>
#include <stdexcept>
using namespace comet::decomp;
int main(){
 float timer=0;
 assert(advance_arena_ground_deployment(timer,{40,0},1)==ArenaGroundDeploymentGate::Inactive && timer==1);
 timer=0;assert(advance_arena_ground_deployment(timer,{40,.5f},1)==ArenaGroundDeploymentGate::Waiting);
 assert(advance_arena_ground_deployment(timer,{40,.5f},.1f)==ArenaGroundDeploymentGate::Ready);
 timer=0;assert(advance_arena_ground_deployment(timer,{45,1},1)==ArenaGroundDeploymentGate::Waiting);
 assert(advance_arena_ground_deployment(timer,{45,1},.01f)==ArenaGroundDeploymentGate::Ready);
 const float before=timer;
 assert(advance_arena_ground_deployment(timer,{40,std::numeric_limits<float>::quiet_NaN()},1)==ArenaGroundDeploymentGate::Invalid && timer==before);
 assert(advance_arena_ground_deployment(timer,{39,1},1)==ArenaGroundDeploymentGate::Invalid && timer==before);
 assert(advance_arena_ground_deployment(timer,{40,1},-1)==ArenaGroundDeploymentGate::Invalid && timer==before);
 assert(arena_ground_deployment_position({3,7})==std::optional(std::array<float,4>{3.5f,0,7.5f,0}));
 assert(!arena_ground_deployment_position({24,0}));
 const auto rounding=std::fegetround();timer=0;advance_arena_ground_deployment(timer,{41,1},.2f);assert(std::fegetround()==rounding);
 {
  ArenaConstructionSimulationConfig c;c.capacity=4;c.seed=4;
  c.players={{{{1,1},0,false},100,100},{{{4,1},1,false},100,100}};
  c.unit_base_available={true,true,false,false};c.ground_transport_enabled=true;c.base_counters[1]=2;
  c.unit_quotas[0].capacity=2;c.unit_quotas[0].pending=2;c.unit_quotas[0].by_type[0]=2;
  c.ground_deployment[0]=ArenaGroundDeploymentControl{40,1};
  ArenaState arena;arena.players.resize(2);arena.players[0].start_grid=ArenaGridPosition{1,1};arena.players[1].start_grid=ArenaGridPosition{4,1};
  auto scene=std::make_unique<NativeConstructionScene>(arena,c);
  auto tick=scene->advance(.1f);assert(tick.ground_deployments.size()==1 && tick.ground_deployments[0].result.entity_index==0);
  assert(scene->simulation().unit_quotas()[0].active==1 && scene->simulation().unit_quotas()[0].pending==1);
  assert(scene->arena().entities.size()==1 && scene->arena().entities[0].position.x>1.3f && scene->arena().entities[0].position.x<1.7f);
  assert(scene->simulation().deployment_timers()[0]==0); // successful release resets, rather than carrying overshoot
  assert(scene->set_ground_deployment(0,{40,0}));scene->advance(.2f);assert(scene->simulation().unit_quotas()[0].pending==1);
  assert(scene->set_ground_deployment(0,{40,1}));scene->advance(.3f);assert(scene->simulation().unit_quotas()[0].active==2);
  bool hit=false;for(unsigned n=4;n<=200 && !hit;++n)hit=!scene->advance(n*.1f).base_impacts.empty();
  assert(hit && scene->simulation().base_counter(1)==1);
  assert(!scene->set_ground_deployment(0,{42,1}) && !scene->set_ground_deployment(2,{40,1}));
 }
 {
  ArenaConstructionSimulationConfig c;c.capacity=0;c.players={{{{1,1},0,false},100,100}};
  c.unit_base_available[0]=true;c.unit_quotas[0].capacity=1;c.unit_quotas[0].pending=c.unit_quotas[0].by_type[0]=1;
  c.ground_deployment[0]=ArenaGroundDeploymentControl{40,1};
  auto sim=std::make_unique<ArenaConstructionSimulation>(c);auto tick=sim->advance(.1f);
  assert(tick.ground_deployments.size()==1 && tick.ground_deployments[0].result.status==ArenaUnitSpawnStatus::AllocationFailed);
  assert(sim->unit_quotas()[0].pending==0 && sim->unit_quotas()[0].active==0 && sim->deployment_timers()[0]>.09f);
  tick=sim->advance(.2f);assert(tick.ground_deployments[0].result.status==ArenaUnitSpawnStatus::NoOrder);
 }
 { // Independent owners release against their own orders and timers.
  ArenaConstructionSimulationConfig c;c.capacity=4;c.players={{{{1,1},0,false},100,100},{{{4,1},1,false},100,100}};
  c.unit_base_available={true,true,false,false};
  for(unsigned owner=0;owner<2;++owner){c.unit_quotas[owner].capacity=1;c.unit_quotas[owner].pending=c.unit_quotas[owner].by_type[0]=1;c.ground_deployment[owner]=ArenaGroundDeploymentControl{40,1};}
  auto sim=std::make_unique<ArenaConstructionSimulation>(c);const auto tick=sim->advance(.1f);
  assert(tick.ground_deployments.size()==2 && tick.ground_deployments[0].owner==0 && tick.ground_deployments[1].owner==1);
  assert(sim->unit_quotas()[0].active==1 && sim->unit_quotas()[1].active==1);
  assert(sim->storage().banks[0][0][0x33]>>5==0 && sim->storage().banks[0][1][0x33]>>5==1);
  const auto pool=sim->storage().free_indices;const auto quotas=sim->unit_quotas();const auto timers=sim->deployment_timers();
  bool rejected=false;try{sim->advance(11);}catch(const std::invalid_argument&){rejected=true;}
  assert(rejected && sim->clock()==.1f && sim->storage().free_indices==pool && sim->deployment_timers()==timers);
  assert(sim->unit_quotas()[0].active==quotas[0].active && sim->unit_quotas()[1].active==quotas[1].active);
 }
 { // Same-tick retirement and deployment reuse must replace the projection.
  ArenaConstructionSimulationConfig c;c.capacity=1;c.seed=4;c.ground_transport_enabled=true;
  c.players={{{{1,1},0,false},100,100},{{{4,1},1,false},100,100}};c.unit_base_available={true,true,false,false};
  c.unit_quotas[0].capacity=2;c.unit_quotas[0].pending=c.unit_quotas[0].by_type[0]=2;c.base_counters[1]=2;
  c.ground_deployment[0]=ArenaGroundDeploymentControl{40,1};
  ArenaState arena;arena.players.resize(2);arena.players[0].start_grid=ArenaGridPosition{1,1};arena.players[1].start_grid=ArenaGridPosition{4,1};
  auto scene=std::make_unique<NativeConstructionScene>(arena,c);assert(scene->spawn_reserved_ground_unit(0,40,{4,1}).entity_index==0);
  const auto tick=scene->advance(.1f);assert(tick.retired_entities==std::vector<std::uint16_t>{0});
  assert(tick.ground_deployments.size()==1 && tick.ground_deployments[0].result.entity_index==0);
  assert(scene->arena().entities.size()==1 && scene->arena().entities[0].live_entity_index==0 && scene->arena().entities[0].position.x<1.7f);
  assert(scene->simulation().unit_quotas()[0].active==1 && scene->simulation().base_counter(1)==1);
 }
 { // A full base cell consumes/compensates the attempted order, not peers.
  ArenaConstructionSimulationConfig c;c.capacity=127;c.players={{{{1,1},0,false},100,100}};c.unit_base_available[0]=true;
  c.unit_quotas[0].capacity=127;c.unit_quotas[0].pending=c.unit_quotas[0].by_type[0]=127;c.ground_deployment[0]=ArenaGroundDeploymentControl{40,1};
  auto sim=std::make_unique<ArenaConstructionSimulation>(c);
  for(unsigned n=0;n<126;++n)assert(sim->spawn_reserved_ground_unit(0,40,{1.5f,0,1.5f,0}).entity_index);
  const auto tick=sim->advance(.1f);assert(tick.ground_deployments[0].result.status==ArenaUnitSpawnStatus::AllocationFailed);
  assert(sim->unit_quotas()[0].active==126 && sim->unit_quotas()[0].pending==0);
  assert(sim->storage().summary_counts[25]==126 && sim->storage().free_indices.size()==1);
 }

}
