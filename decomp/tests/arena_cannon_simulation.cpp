#include "comet/native_construction_scene.hpp"
#include "comet/native_mission_playback.hpp"
#include <bit>
#include <cassert>
#include <memory>
#include <stdexcept>
using namespace comet::decomp;
namespace {
float scalar(const ArenaEntitySlot&s,unsigned o){unsigned w=0;for(unsigned i=0;i<4;i++)w=(w<<8)|s[o+i];return std::bit_cast<float>(w);}
std::unique_ptr<NativeConstructionScene> setup(bool enabled,unsigned capacity=16,bool allied=false){
 ArenaConstructionSimulationConfig c;c.capacity=capacity;c.seed=4;c.cannon_enabled=enabled;
 c.ground_dynamics_enabled=!enabled;c.ground_contacts_enabled=false;
 c.players={{{{0,6},0,false},400,400},{{{23,6},1,false},400,400}};
 if(allied)c.players[1].route.team=0;
 c.tuning[21]=ArenaStructureTableValues{100,50,40,20,7.5f};
 c.unit_base_available[0]=c.unit_base_available[1]=true;
 c.unit_quotas[1].capacity=2;c.unit_quotas[1].pending=c.unit_quotas[1].by_type[1]=2;
 ArenaState a;a.players.resize(2);a.players[0].start_grid=ArenaGridPosition{0,6};a.players[1].start_grid=ArenaGridPosition{23,6};
 auto scene=std::make_unique<NativeConstructionScene>(a,c);
 assert(!enabled || scene->simulation().ground_dynamics_enabled());
 assert(scene->enqueue({0,21,6,6})==ArenaConstructionSubmit::Queued);scene->step();scene->advance(2);
 assert(scene->spawn_reserved_ground_unit(1,41,{9,6}).entity_index==1);
 scene->set_ground_transport(true);return scene;
}
}
int main(){
 auto scene=setup(true),control=setup(false);
 bool fired=false,hit=false,projected=false;
 for(unsigned n=1;n<=300;n++){
  const auto tick=scene->advance(2.f+float(n)*.02f);control->advance(2.f+float(n)*.02f);
  fired|=!tick.spawned_projectiles.empty();hit|=!tick.cannon_impacts.empty();
  for(const auto&e:scene->arena().entities)projected|=e.type==3;
 }
 assert(fired && hit && projected);
 // Every projected shot follows the authoritative live slot, including migration.
 for(const auto&e:scene->arena().entities)if(e.type==3){
  const auto &slot=scene->simulation().storage().banks[0][*e.live_entity_index];
  assert(e.position.x==scalar(slot,0) && e.position.y==scalar(slot,4) && e.position.z==scalar(slot,8));
 }
 const auto &bank=scene->simulation().storage().banks[0];
 assert(bank[1][15]==41 && scalar(bank[1],0x38)==170.f); // Kinetic impacts do not invent damage.
 assert(bank[1]!=control->simulation().storage().banks[0][1]);
 assert(scene->simulation().unit_quotas()[1].active==1);
 assert(scene->simulation().storage().banks[0]==scene->simulation().storage().banks[1]);
 // Disabled and allied sources never allocate a projectile.
 for(const auto &slot:control->simulation().storage().banks[0])assert(slot[15]!=3);
 auto allied=setup(true,16,true),full=setup(true,2);
 for(unsigned n=1;n<=150;n++){
  const auto a=allied->advance(2.f+float(n)*.02f),f=full->advance(2.f+float(n)*.02f);
  assert(a.spawned_projectiles.empty() && f.spawned_projectiles.empty());
 }
 assert(full->simulation().storage().free_indices.empty());
 assert(full->simulation().unit_quotas()[1].active==1);
 // Snapshot immediately after a hit includes pending knockback and shared RNG.
 auto combat=setup(true);bool pending_hit=false;
 for(unsigned n=1;n<=300 && !pending_hit;n++)pending_hit=!combat->advance(2.f+float(n)*.02f).cannon_impacts.empty();
 assert(pending_hit);
 auto saved=std::make_unique<NativeConstructionScene>(*combat);
 NativeMissionPlayback playback(*combat);
 for(unsigned n=1;n<=100;n++)playback.advance(.02f);
 const auto final_banks=combat->simulation().storage().banks;
 const auto final_cells=combat->simulation().storage().cells;
 const auto final_pool=combat->simulation().storage().free_indices;
 playback.restart();
 assert(combat->simulation().storage().banks==saved->simulation().storage().banks);
 for(unsigned n=1;n<=100;n++)playback.advance(.02f);
 assert(combat->simulation().storage().banks==final_banks);
 assert(combat->simulation().storage().cells==final_cells);
 assert(combat->simulation().storage().free_indices==final_pool);
 auto paused=std::make_unique<NativeConstructionScene>(*combat);
 paused->set_ground_transport(false);const auto frozen=paused->simulation().storage().banks;
 paused->advance(paused->simulation().clock()+1);assert(paused->simulation().storage().banks==frozen);
 const auto clock=combat->simulation().clock();bool rejected=false;
 try{combat->advance(clock+11);}catch(const std::invalid_argument&){rejected=true;}
 assert(rejected && combat->simulation().clock()==clock && combat->simulation().storage().banks==final_banks);

 // A separate restart checkpoint explicitly contains an in-flight shot.
 auto flying=setup(true);bool in_flight=false;
 for(unsigned n=1;n<=300 && !in_flight;n++){
  flying->advance(2.f+float(n)*.02f);
  for(const auto &slot:flying->simulation().storage().banks[0])in_flight|=slot[15]==3;
 }
 assert(in_flight);NativeMissionPlayback flight_playback(*flying);
 for(unsigned n=0;n<30;n++)flight_playback.advance(.02f);
 const auto flight_final=flying->simulation().storage().banks;
 flight_playback.restart();
 for(unsigned n=0;n<30;n++)flight_playback.advance(.02f);
 assert(flying->simulation().storage().banks==flight_final);

}
