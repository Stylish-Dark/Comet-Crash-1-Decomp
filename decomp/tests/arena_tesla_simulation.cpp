#include "comet/native_construction_scene.hpp"
#include "comet/native_mission_playback.hpp"
#include "comet/arena_construction_config.hpp"
#include "../src/arena_tesla_runtime.hpp"
#include <bit>
#include <cassert>
#include <memory>
#include <sstream>
#include <stdexcept>
using namespace comet::decomp;
namespace {
float value(const ArenaEntitySlot&s,unsigned o){unsigned w=0;for(unsigned i=0;i<4;i++)w=(w<<8)|s[o+i];return std::bit_cast<float>(w);}
ArenaConstructionSimulationConfig config(){
 ArenaConstructionSimulationConfig c;c.capacity=8;c.seed=4;c.tesla_enabled=true;
 c.players={{{{0,0},0,false},400,400},{{{12,12},1,false},400,400}};
 c.tuning[22]=ArenaStructureTableValues{100,50,30,20,7.5f};
 c.unit_base_available[1]=true;
 c.unit_quotas[1].capacity=2;c.unit_quotas[1].pending=2;
 c.unit_quotas[1].by_type[1]=1;c.unit_quotas[1].by_type[6]=1;
 return c;
}
std::unique_ptr<NativeConstructionScene> setup(ArenaConstructionSimulationConfig c,std::uint8_t kind=41,ArenaGridPosition cell={5,4}){
 ArenaState a;a.players.resize(2);a.players[0].start_grid=ArenaGridPosition{0,0};a.players[1].start_grid=ArenaGridPosition{12,12};
 auto scene=std::make_unique<NativeConstructionScene>(a,c);
 assert(scene->enqueue({0,22,4,4})==ArenaConstructionSubmit::Queued);scene->step();scene->advance(2);
 assert(scene->spawn_reserved_ground_unit(1,kind,cell).entity_index==1);
 scene->set_ground_transport(true);return scene;
}
const ArenaEntitySlot &slot(const NativeConstructionScene&s,unsigned i){return s.simulation().storage().banks[0][i];}
unsigned beams(const NativeConstructionScene&s){unsigned n=0;for(const auto&b:s.simulation().storage().banks[0])n+=b[15]==5;return n;}
}
int main(){
 auto scene=setup(config());
 auto checkpoint=std::make_unique<NativeMissionPlayback>(*scene);
 bool beam=false,damaged=false,depleted=false,projected=false;
 for(unsigned n=1;n<=240;n++){
  const auto tick=scene->advance(2.f+float(n)*.025f);
  beam|=beams(*scene)>0;damaged|=value(slot(*scene,1),0x38)<170.f;
  depleted|=!tick.depleted_units.empty();
  for(const auto&e:scene->arena().entities)projected|=e.beam_path.has_value();
 }
 assert(beam && damaged && depleted && projected);
 assert(scene->simulation().unit_quotas()[1].active==0);
 assert(scene->simulation().storage().banks[0]==scene->simulation().storage().banks[1]);
 const auto final=scene->simulation().storage().banks;
 checkpoint->restart();scene->set_ground_transport(true);
 for(unsigned n=1;n<=240;n++)scene->advance(2.f+float(n)*.025f);
 assert(scene->simulation().storage().banks==final);
 // Asymmetric route bytes prove candidate-to-tower direction. A second pass
 // moves the best broad candidate out of the local 3x3: local acquisition wins.
 for(bool broad:{false,true}){
  auto source=setup(config());auto world=source->simulation().storage();auto grid=source->simulation().grid();
  auto second=world.banks[0][1];
  const ArenaGridPosition cell=broad?ArenaGridPosition{7,4}:ArenaGridPosition{4,5};
  auto put=[&](unsigned o,float f){const auto bits=std::bit_cast<unsigned>(f);for(unsigned k=0;k<4;k++)second[o+k]=bits>>(24-8*k);};
  put(0,float(cell.x)+.5f);put(8,float(cell.z)+.5f);
  assert(insert_arena_entity(world,cell,second)==2);
  grid.cells[4*24+5].routes[3]=0xf0;grid.cells[4*24+5].routes[0]=0;
  grid.cells[cell.z*24+cell.x].routes[3]=0;grid.cells[cell.z*24+cell.x].routes[0]=0xf0;
  ArenaRandom rng(4);std::vector<float> health(8);std::vector<std::uint32_t> commands(8);
  const std::array<std::uint8_t,2> teams{0,1};
  stage_arena_teslas(world,grid,teams,.025f,rng,health,commands);
  assert(world.banks[0][0][0x91]==(broad?1:2));
 }
 // Disabled firing and allied units never receive automatic damage.
 for(unsigned mode=0;mode<2;mode++){
  auto c=config();if(mode==0)c.tesla_enabled=false;else c.players[1].route.team=0;
  auto other=setup(c);other->advance(2.5f);other->advance(3.f);
  assert(value(slot(*other,1),0x38)==170.f);
  if(mode==0)assert(!beams(*other));
  else assert(slot(*other,0)[0x90]==255 && slot(*other,0)[0x91]==255);
 }
 // A full pool cannot manufacture an active beam or acquire a target.
 auto full_config=config();full_config.capacity=2;auto full=setup(full_config);
 full->advance(2.5f);full->advance(3.f);
 assert(!beams(*full) && value(slot(*full,1),0x38)==170.f);
 assert(slot(*full,0)[0xb9]==0 && slot(*full,0)[0x90]==255 && slot(*full,0)[0x91]==255);
 // Broader target gathering may acquire a unit beyond the strict 2.5 hit range.
 auto far=setup(config(),41,{7,4});far->advance(2.5f);far->advance(3.f);
 assert(beams(*far)==1 && slot(*far,0)[0x91]==1 && value(slot(*far,1),0x38)==170.f);
 // Kind46 triggers the original extra damage and four-second tower shutdown.
 auto medic=setup(config(),46);bool medic_dead=false,retired=false;
 for(unsigned n=1;n<=24;n++){
  const auto tick=medic->advance(2.f+float(n)*.025f);
  medic_dead|=!tick.depleted_units.empty();retired|=!tick.retired_entities.empty();
 }
 assert(medic_dead && retired && !beams(*medic) && value(slot(*medic,0),0xc8)>3.f);
 auto cooldown_checkpoint=std::make_unique<NativeMissionPlayback>(*medic);
 for(unsigned n=25;n<=220;n++)medic->advance(2.f+float(n)*.025f);
 const auto restarted=medic->simulation().storage().banks;
 assert(beams(*medic)==1);
 cooldown_checkpoint->restart();medic->set_ground_transport(true);
 for(unsigned n=25;n<=220;n++)medic->advance(2.f+float(n)*.025f);
 assert(medic->simulation().storage().banks==restarted);
 // Pausing transport freezes the weapon state; rejected clocks roll back it all.
 auto paused=setup(config());paused->advance(2.1f);paused->set_ground_transport(false);
 const auto frozen=paused->simulation().storage().banks;paused->advance(2.5f);
 assert(paused->simulation().storage().banks==frozen);
 bool rejected=false;try{paused->advance(2.4f);}catch(const std::invalid_argument&){rejected=true;}
 assert(rejected && paused->simulation().storage().banks==frozen);
 std::size_t line=0;std::istringstream in("world 8 4\nplayer 0 0 0 400 400 0\ntesla 1\nrun\n");
 assert(read_arena_construction_config(in,line).tesla_enabled);
 for(const auto *text:{"tesla 2\n","tesla 1\ntesla 0\n"}){
  std::istringstream bad(std::string("world 8 4\nplayer 0 0 0 400 400 0\n")+text+"run\n");
  rejected=false;try{read_arena_construction_config(bad,line);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);
 }
}
