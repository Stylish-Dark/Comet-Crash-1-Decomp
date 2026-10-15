#include "comet/native_construction_scene.hpp"
#include "comet/native_mission_playback.hpp"
#include <cassert>
#include <bit>
#include <cmath>
#include <limits>
#include <memory>
using namespace comet::decomp;
float scalar(const ArenaEntitySlot&s,unsigned o){unsigned w=0;for(unsigned i=0;i<4;++i)w=(w<<8)|s[o+i];return std::bit_cast<float>(w);}
int main(){
 ArenaConstructionSimulationConfig c;c.capacity=1;c.seed=4;c.ground_transport_enabled=true;
 c.players={{{{0,0},0,false},100,100},{{{10,10},1,false},100,100}};
 c.unit_base_available[0]=c.unit_base_available[1]=true;
 c.unit_quotas[0].capacity=2;c.unit_quotas[0].pending=2;c.unit_quotas[0].by_type[0]=2;
 ArenaState a;a.players.resize(2);a.players[0].start_grid=ArenaGridPosition{0,0};a.players[1].start_grid=ArenaGridPosition{10,10};
 auto scene=std::make_unique<NativeConstructionScene>(a,c);
 assert(!scene->queue_ground_health_delta(0,-10)); // No live target.
 assert(scene->spawn_reserved_ground_unit(0,40,{3,3}).entity_index==0);
 assert(!scene->queue_ground_health_delta(65535,-1));
 assert(!scene->queue_ground_health_delta(0,std::numeric_limits<float>::infinity()));
 assert(scene->queue_ground_health_delta(0,-20));assert(scene->queue_ground_health_delta(0,5));
 NativeMissionPlayback checkpoint(*scene); // Restart must restore pending effects too.
 scene->advance(0);assert(scalar(scene->simulation().storage().banks[0][0],0x38)==50);
 scene->advance(.01f);assert(scalar(scene->simulation().storage().banks[0][0],0x38)==35);
 scene->advance(.02f);assert(scalar(scene->simulation().storage().banks[0][0],0x38)==35); // Consumed once.
 checkpoint.restart();scene->advance(.01f);assert(scalar(scene->simulation().storage().banks[0][0],0x38)==35);
 assert(scene->queue_ground_health_delta(0,100));
 scene->advance(.02f);assert(scalar(scene->simulation().storage().banks[0][0],0x38)==50);
 assert(scene->queue_ground_health_delta(0,-500));
 const auto before=scene->simulation().storage().banks;
 bool rejected=false;try{scene->advance(100);}catch(const std::invalid_argument&){rejected=true;}
 assert(rejected && scene->simulation().storage().banks==before); // Rejection retains queued hit.
 const auto death=scene->advance(.03f);assert(death.depleted_units==std::vector<std::uint16_t>{0});
 assert(scene->simulation().storage().banks[0][0][15]==62);
 assert(scene->simulation().unit_quotas()[0].active==0 && scene->arena().entities[0].corpse_fraction);
 assert(!scene->queue_ground_health_delta(0,100)); // Dead corpse is not a health target.
 scene->advance(2);assert(scene->arena().entities.empty());
 assert(scene->spawn_reserved_ground_unit(0,40,{3,3}).entity_index==0);
 scene->advance(2.01f);assert(scalar(scene->simulation().storage().banks[0][0],0x38)==50); // No stale hit after reuse.
 assert(scene->simulation().storage().banks[0]==scene->simulation().storage().banks[1]);
 // Motion-off previews retain effects; enabling transport consumes them once.
 scene->set_ground_transport(false);assert(scene->queue_ground_health_delta(0,-10));scene->advance(3);
 assert(scalar(scene->simulation().storage().banks[0][0],0x38)==50);
 scene->set_ground_transport(true);scene->advance(3.01f);assert(scalar(scene->simulation().storage().banks[0][0],0x38)==40);

 // Converter activation must arise from its own original pulse, no manual flag.
 c.capacity=4;c.unit_quotas[0].pending=c.unit_quotas[0].by_type[0]=1;
 c.unit_quotas[1].capacity=2;c.unit_quotas[1].pending=c.unit_quotas[1].by_type[5]=1;
 c.ground_contacts_enabled=true;
 auto encounter=std::make_unique<NativeConstructionScene>(a,c);
 assert(encounter->spawn_reserved_ground_unit(1,45,{3,3}).entity_index==0);
 assert(encounter->simulation().storage().banks[0][0][0xbb]==0);
 encounter->advance(.01f);assert(encounter->simulation().storage().banks[0][0][0xbb]==0);
 encounter->advance(.10f);
 assert(encounter->simulation().storage().banks[0][0][0xbb]==1);
 // Introduce the victim during the active pulse. An earlier inactive collision
 // correctly separates units before the converter becomes ready.
 assert(encounter->spawn_reserved_ground_unit(0,40,{3,3}).entity_index==1);
 encounter->advance(.11f);
 assert(encounter->arena().entities[1].owner==1);
 assert(encounter->simulation().unit_quotas()[0].active==0 && encounter->simulation().unit_quotas()[1].active==2);
 const auto& converter=encounter->simulation().storage().banks[0][0];
 assert(scalar(converter,0x98)>0 && scalar(converter,0x18)>=.7f);
 assert(encounter->queue_ground_health_delta(0,-2000));encounter->advance(.12f);
 assert(encounter->simulation().storage().banks[0][0][15]==62);
 assert(encounter->simulation().unit_quotas()[1].active==1);
 // NoTarget/Blocked motion must not truncate the independent converter clock.
 c.unit_base_available={};
 auto idle=std::make_unique<NativeConstructionScene>(a,c);
 assert(idle->spawn_reserved_ground_unit(1,45,{3,3}).entity_index==0);
 idle->advance(.5f);
 assert(scalar(idle->simulation().storage().banks[0][0],0x98)>.9f);
}
