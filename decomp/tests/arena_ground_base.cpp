#include "comet/arena_ground_base.hpp"
#include "comet/arena_player_statistics.hpp"
#include "comet/arena_unit_entity.hpp"
#include "comet/native_construction_scene.hpp"
#include <cassert>
#include <bit>
#include <limits>
#include <memory>
using namespace comet::decomp;
template<std::size_t N> void put(std::array<std::uint8_t,N>&s,unsigned o,unsigned v,unsigned n=4){for(unsigned i=0;i<n;++i)s[o+i]=v>>(8*(n-1-i));}
template<std::size_t N> unsigned get(const std::array<std::uint8_t,N>&s,unsigned o,unsigned n=4){unsigned v=0;for(unsigned i=0;i<n;++i)v=(v<<8)|s[o+i];return v;}
int main(){
 {
  ArenaConstructionSimulationConfig c;c.capacity=3;c.seed=4;
  c.players={{{{0,0},0,false},100,100},{{{4,4},1,false},100,100}};
  c.unit_base_available[0]=c.unit_base_available[1]=true;
  c.base_counters[1]=2;
  c.unit_quotas[0].capacity=2;c.unit_quotas[0].pending=2;c.unit_quotas[0].by_type[0]=2;
  c.ground_transport_enabled=true;c.tuning[20]=ArenaStructureTableValues{100,50,10,20,7.5f};
  ArenaState arena;arena.players.resize(2);arena.players[0].start_grid=ArenaGridPosition{0,0};arena.players[1].start_grid=ArenaGridPosition{4,4};
  auto scene=std::make_unique<NativeConstructionScene>(arena,c);
  const auto first=scene->spawn_reserved_ground_unit(0,40,{3,4});assert(first.entity_index==0);
  assert(scene->enqueue({0,20,8,8})==ArenaConstructionSubmit::Queued);assert(scene->step().entity_index==1);
  bool hit=false;
  for(unsigned n=1;n<=200 && !hit;++n){auto tick=scene->advance(n*.05f);if(!tick.base_impacts.empty()){assert(tick.base_impacts.size()==1 && tick.base_impacts[0].base==1 && tick.base_impacts[0].owner==0);hit=true;}}
  assert(hit && scene->simulation().base_counter(1)==1 && scene->simulation().unit_quotas()[0].active==0);
  assert(scene->arena().entities.size()==1 && scene->arena().cells[8*24+8].entity_index==0);
  assert(scene->simulation().storage().free_indices==std::deque<std::uint32_t>({2,0}));
  const auto second=scene->spawn_reserved_ground_unit(0,40,{4,4});assert(second.entity_index==2);
  assert(scene->enqueue({0,20,9,9})==ArenaConstructionSubmit::Queued); // same-tick reuse of the retired index
  const auto tick=scene->advance(scene->simulation().clock()+.01f);
  assert(tick.base_impacts.size()==1 && tick.construction.entity_index==0);
  assert(scene->simulation().base_counter(1)==0 && scene->simulation().unit_quotas()[0].active==0);
  assert(scene->arena().entities.size()==2 && scene->arena().entities[1].type==20);
  assert(scene->arena().cells[9*24+9].entity_index==1);
  scene->advance(scene->simulation().clock()+.01f);assert(scene->simulation().base_counter(1)==0);
 }

 std::array<ArenaGroundBase,4> bases{};
 bases[0]={{3,3},0,true};bases[1]={{3,3},1,true};bases[2]={{3,3},2,false};bases[3]={{3,3},3,true};
 assert(select_arena_ground_base({3.5f,0,3.5f,0},0,1,bases)==3);
 bases[3].available=false;
 assert(select_arena_ground_base({3.5f,0,3.5f,0},0,1,bases)==1);
 assert(!select_arena_ground_base({3.f,0,3.5f,0},0,1,bases)); // strict half-cell radius
 assert(!select_arena_ground_base({3.5f,.5f,3.5f,0},0,1,bases));
 assert(select_arena_ground_base({3.5f,.49f,3.5f,0},0,1,bases)==1);
 assert(!select_arena_ground_base({3.5f,0,3.5f,0},0,0,bases));
 assert(!select_arena_ground_base({std::numeric_limits<float>::quiet_NaN(),0,0,0},0,1,bases));
 std::array<std::uint8_t,128> stats{};put(stats,88,1,2);put(stats,120,9,2);
 ArenaPlayerResourceDelta d;d.base_counter=-1;d.attribution=2;d.base_counter_enabled=true;
 apply_arena_player_resource_delta(stats,d);assert(get(stats,88,2)==0 && get(stats,120,2)==2);
 d.attribution=3;apply_arena_player_resource_delta(stats,d);assert(get(stats,88,2)==0 && get(stats,120,2)==2);
 put(stats,88,5,2);d.base_counter_enabled=false;apply_arena_player_resource_delta(stats,d);assert(get(stats,88,2)==5 && get(stats,120,2)==2);
 put(stats,72,std::bit_cast<unsigned>(9.f));put(stats,76,std::bit_cast<unsigned>(10.f));put(stats,84,9999,2);
 d.resource=5;d.capacity=2;d.count_84=10;apply_arena_player_resource_delta(stats,d);
 assert(std::bit_cast<float>(get(stats,72))==12 && get(stats,84,2)==10000);
 auto storage=std::make_unique<ArenaEntityStorage>();for(auto &b:storage->banks)b.resize(3);reset_arena_entity_index_pool(*storage,3);
 ArenaEntitySlot slot{};ArenaRandom rng(1);assert(initialize_arena_unit_template(slot,40,0,{3.5f,0,3.5f,0},rng));
 assert(insert_arena_entity(*storage,{3,3},slot)==0);assert(insert_arena_entity(*storage,{3,3},slot)==1);
 assert(retire_arena_entity(*storage,0));assert(storage->free_indices==std::deque<std::uint32_t>({2,0}));
 assert(storage->summary_counts[75]==1 && storage->cells[0][75].size()==1 && storage->cells[0][75][0][1]==1);
 assert(storage->banks[0][0]==ArenaEntitySlot{} && storage->banks[1][0]==ArenaEntitySlot{});
 assert(!retire_arena_entity(*storage,0));assert(storage->free_indices.size()==2);
 storage->cells[1][75].clear();const auto before=storage->banks;const auto pool=storage->free_indices;
 assert(!retire_arena_entity(*storage,1));assert(storage->banks==before && storage->free_indices==pool && storage->summary_counts[75]==1);
}
