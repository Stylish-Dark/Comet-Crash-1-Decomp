#include "comet/arena_ground_capture.hpp"
#include "comet/arena_player_statistics.hpp"
#include "comet/arena_unit_entity.hpp"
#include "comet/native_construction_scene.hpp"
#include <cassert>
#include <bit>
#include <limits>
#include <stdexcept>
using namespace comet::decomp;
void put(ArenaEntitySlot&s,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)s[o+i]=v>>(24-8*i);}
struct Fixture {
 ArenaEntitySlot victim{},peer{};
 std::array<std::uint8_t,2> teams{0,1};
 std::array<ArenaPlayerStatisticsBlock,2> stats{};
 Fixture(){
  ArenaRandom rng(4);assert(initialize_arena_unit_template(victim,40,0,{3,0,3,0},rng));
  assert(initialize_arena_unit_template(peer,45,1,{3.1f,0,3,0},rng));
  peer[0xbb]=1;peer[0x7e]=0;victim[0x7e]=1;victim[0x1e]=0x12;victim[0x1f]=0x34;
  ArenaUnitProductionQuota q;q.capacity=8;store_arena_unit_production_quota(stats[0],q);store_arena_unit_production_quota(stats[1],q);
  assert(register_arena_entity_statistics(stats[0],victim,true,true)==ArenaEntityRegistration::Registered);
 }
 ArenaGroundCapture run(bool overlap=true){return capture_arena_ground_unit(victim,peer,teams,stats,overlap);}
};
int main(){
 {
  ArenaConstructionSimulationConfig c;c.capacity=4;c.seed=4;
  c.players={{{{0,0},0,false},100,100},{{{10,10},1,false},100,100}};
  c.unit_base_available[0]=c.unit_base_available[1]=true;
  c.unit_quotas[0].capacity=c.unit_quotas[1].capacity=8;
  c.unit_quotas[0].pending=c.unit_quotas[0].by_type[0]=1;
  c.unit_quotas[1].pending=c.unit_quotas[1].by_type[5]=1;
  c.ground_transport_enabled=c.ground_contacts_enabled=true;
  ArenaState arena;arena.players.resize(2);arena.players[0].start_grid=ArenaGridPosition{0,0};arena.players[1].start_grid=ArenaGridPosition{10,10};
  NativeConstructionScene scene(arena,c);
  const auto victim=scene.spawn_reserved_ground_unit(0,40,{3,3}),peer=scene.spawn_reserved_ground_unit(1,45,{3,3});assert(victim.entity_index && peer.entity_index);
  assert(!scene.set_ground_capture_ready(*victim.entity_index,true));
  assert(!scene.set_ground_capture_ready(60000,true));
  assert(scene.set_ground_capture_ready(*peer.entity_index,true));
  scene.advance(.01f);
  assert(scene.simulation().unit_quotas()[0].active==0 && scene.simulation().unit_quotas()[1].active==2);
  assert(scene.arena().entities[0].owner==1);
  scene.advance(.02f);assert(scene.simulation().unit_quotas()[1].active==2); // no duplicate transfer
 }

 {
  Fixture f;const auto before=f.victim;assert(f.run()==ArenaGroundCapture::Captured);
  assert(f.victim[0x33]==(before[0x33]&31u)+32u && f.victim[0x7e]==0);
  assert(f.victim[0x1e]==255 && f.victim[0x1f]==255);
  for(unsigned i=0;i<256;++i)if(i!=0x33 && i!=0x7e && i!=0x1e && i!=0x1f)assert(f.victim[i]==before[i]);
  assert(read_arena_unit_production_quota(f.stats[0]).active==0);
  assert(read_arena_unit_production_quota(f.stats[1]).active==1);
 }
 {
  Fixture f;put(f.victim,0x50,std::bit_cast<unsigned>(std::numeric_limits<float>::quiet_NaN()));
  const auto before=f.victim;const auto stats=f.stats;assert(f.run()==ArenaGroundCapture::Invalid);assert(f.victim==before && f.stats==stats);
 }
 for(unsigned mode=0;mode<5;++mode){
  Fixture f;if(mode==1)f.teams[1]=0;if(mode==2)f.peer[15]=44;if(mode==3)f.peer[0xbb]=0;if(mode==4)put(f.peer,0x60,0x20000);
  auto old=f.victim;auto stats=f.stats;assert(f.run(mode!=0)==ArenaGroundCapture::NotEligible);assert(f.victim==old && f.stats==stats);
 }
 {
  Fixture f;auto q=read_arena_unit_production_quota(f.stats[1]);q.capacity=0;store_arena_unit_production_quota(f.stats[1],q);
  auto old=f.victim;assert(f.run()==ArenaGroundCapture::CapacityRejected);assert(f.victim==old);
  assert(read_arena_unit_production_quota(f.stats[0]).active==1 && read_arena_unit_production_quota(f.stats[1]).active==0);
 }
 {
  Fixture f;f.victim[15]=45;auto old=f.victim;auto stats=f.stats;assert(f.run()==ArenaGroundCapture::Unsupported);assert(f.victim==old && f.stats==stats);
 }
 {
  Fixture f;f.peer[0x33]=0xf4;auto old=f.victim;auto stats=f.stats;assert(f.run()==ArenaGroundCapture::Invalid);assert(f.victim==old && f.stats==stats);
 }
}
