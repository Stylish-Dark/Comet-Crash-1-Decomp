#include "comet/native_session.hpp"
#include "comet/native_construction_scene.hpp"
#include "comet/native_mission_playback.hpp"
#include "comet/native_deployment_input.hpp"
#include <cassert>
#include <memory>
#include <fstream>
#include "comet/arena_construction_config.hpp"
using namespace comet::decomp;
static ArenaConstructionSimulationConfig config() {
 ArenaConstructionSimulationConfig c;c.capacity=64;c.seed=4;
 c.players={{{{2,12},0,false},400,400},{{{21,12},1,false},400,400}};
 c.production_enabled=true;c.ground_transport_enabled=true;
 c.unit_base_available[0]=c.unit_base_available[1]=true;
 c.unit_quotas[0].capacity=c.unit_quotas[1].capacity=4;
 c.base_counters[0]=c.base_counters[1]=2;c.native_opponents[1]=true;
 c.tuning[26]=ArenaStructureTableValues{100,50,10,20,7.5f};
 return c;
}
int main(int argc,char **argv) {
 NativeSession session;auto c=config();
 assert(initialize_native_skirmish({},c,session));
 assert(session.skirmish && session.arena.extent==24 && session.arena.players.size()==2);
 assert(session.level.primary_records.empty() && session.arena.entities.size()==2);
 for(unsigned owner=0;owner<2;++owner) {
  const auto &base=session.arena.entities[owner];
  assert(base.type==29 && base.owner==owner && base.model_slot==21);
  assert(base.grid==c.players[owner].route.base && !base.live_entity_index);
 }
 assert(session.model_assets.size()==37 && !session.targets.textures.empty());
 auto scene=std::make_unique<NativeConstructionScene>(session.arena,c);
 assert(scene->imports().live==0 && scene->imports().display_only==2);
 assert(scene->enqueue({0,26,2,12})==ArenaConstructionSubmit::Occupied);
 assert(scene->enqueue({0,26,3,12})==ArenaConstructionSubmit::Queued);scene->step();
 NativeMissionPlayback match(*scene);NativeDeploymentInput input(*scene);input.press();
 bool human_spawned=false,opponent_spawned=false;
 for(unsigned i=0;i<1200 && !match.finished();++i) {
  match.advance(.1f);
  human_spawned|=scene->simulation().unit_quotas()[0].active>0;
  opponent_spawned|=scene->simulation().unit_quotas()[1].active>0;
 }
 assert(human_spawned && opponent_spawned && match.finished());
 const auto ended=scene->simulation().storage().banks;match.advance(1);
 assert(scene->simulation().storage().banks==ended);
 input.cancel();match.restart();
 assert(scene->simulation().clock()==0 && !match.finished());
 assert(scene->simulation().accounts()[0].resource==340);
 // Restart restores the launch checkpoint, including the player-built factory.
 assert(scene->arena().entities.size()==3);
 // Invalid startup and campaign transitions leave the existing session intact.
 assert(!change_native_level(1,session));assert(session.skirmish && session.arena.entities.size()==2);
 for(unsigned mode=0;mode<9;++mode) {
  auto invalid=c;NativeSessionConfig render;
  if(mode==0)invalid.native_extent=25;
  if(mode==1)invalid.players[1].route.base=invalid.players[0].route.base;
  if(mode==2)invalid.base_counters[1].reset();
  if(mode==3)invalid.players[1].route.team=0;
  if(mode==4)invalid.unit_base_available[1]=false;
  if(mode==5)invalid.production_enabled=false;
  if(mode==6)render.display_width=0;
  if(mode==7)invalid.initial_visibility[0]=1;
  if(mode==8)invalid.players[0].route.team=4;
  assert(!initialize_native_skirmish(render,invalid,session));
  assert(session.skirmish && session.arena.extent==24 && session.arena.entities.size()==2);
 }
 // Exercise the exact shipped config, including contacts/cannon dynamics defaults.
 assert(argc==2);std::ifstream example(argv[1]);std::size_t line=0;
 const auto shipped=read_arena_construction_config(example,line);
 NativeSession playable;assert(initialize_native_skirmish({},shipped,playable));
 auto encounter=std::make_unique<NativeConstructionScene>(playable.arena,shipped);
 NativeMissionPlayback preview(*encounter);
 for(unsigned i=0;i<2000 && !preview.finished();++i)preview.advance(.1f);
 assert(preview.finished() && encounter->simulation().base_counter(0)==0);

}
