#define SDL_MAIN_HANDLED
#include "comet/native_viewer.hpp"
#include "comet/native_session.hpp"
#include "comet/native_mission_outcome.hpp"
#include <SDL.h>
#include <atomic>
#include <thread>
#include <stdexcept>
#include <memory>
using namespace comet::decomp;
int main() {
 ArenaConstructionSimulationConfig c;c.capacity=32;c.seed=4;
 c.players={{{{1,1},0,false},400,400},{{{4,1},1,false},400,400}};
 c.unit_base_available[0]=c.unit_base_available[1]=true;
 c.unit_quotas[0].capacity=c.unit_quotas[1].capacity=2;
 c.base_counters[0]=c.base_counters[1]=2;
 c.production_enabled=true;c.ground_transport_enabled=true;c.native_opponents[0]=true;
 c.tuning[26]=ArenaStructureTableValues{100,50,10,20,7.5f};
 NativeSession session;
 if(!initialize_native_skirmish({},c,session))throw std::runtime_error("skirmish startup failed");
 auto scene=std::make_unique<NativeConstructionScene>(session.arena,c);
 std::vector<ArenaNativeModel> models;
 for(unsigned slot:{2,21,24,37}) {
  ArenaNativeModel model{arena_model_asset_manifest()[0],{}};model.spec.original_slot_index=slot;
  model.model.vertices={{{-.4f,0,-.4f},{0,1,0},0,0},{{.4f,0,-.4f},{0,1,0},1,0},{{0,.5f,.4f},{0,1,0},.5f,1}};
  model.model.indices={0,1,2};model.model.bounds_min={-.4f,0,-.4f};model.model.bounds_max={.4f,.5f,.4f};
  NativeSubmesh mesh;mesh.range={0,3,0,2};mesh.material.properties.diffuse={.8f,.4f,.2f,0};
  model.model.submeshes={mesh};models.push_back(model);
 }
 NativeViewerConfig view;view.width=320;view.height=240;view.hidden=true;view.construction=scene.get();
 std::atomic<bool> done=false;
 std::thread keyboard([&] {
  while(!SDL_WasInit(SDL_INIT_VIDEO)){if(done.load())return;SDL_Delay(1);}
  SDL_Delay(100);
  auto key=[](SDL_Keycode sym){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=sym;SDL_PushEvent(&e);};
  // The first owner is AI: Tab must retain human owner 1 and the cursor starts at its base.
  key(SDLK_TAB);key(SDLK_RIGHT);key(SDLK_7);key(SDLK_b);key(SDLK_d);
  for(unsigned i=0;i<30;++i){SDL_Delay(30);key(SDLK_t);}
  SDL_Delay(30);SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);
 });
 const auto rendered=run_native_viewer(models,view);done.store(true);keyboard.join();
 if(!rendered)throw std::runtime_error(rendered.detail);
 bool human_factory=false;
 for(const auto &entity:scene->arena().entities)
  human_factory|=entity.type==26 && entity.owner==1 && entity.grid==ArenaGridPosition{5,1};
 if(!human_factory || scene->simulation().accounts()[1].resource!=340 || scene->simulation().accounts()[0].resource!=340)
  throw std::runtime_error("viewer did not build for the selected human at its base cursor");
 const auto result=evaluate_native_mission(scene->simulation(),1);
 if(result!=NativeMissionOutcome::Victory && result!=NativeMissionOutcome::Defeat && result!=NativeMissionOutcome::Draw)
  throw std::runtime_error("keyboard skirmish did not reach a mission result");
 // F2 must restore the selected human and its base cursor, not owner/cell zero.
 scene=std::make_unique<NativeConstructionScene>(session.arena,c);view.construction=scene.get();done.store(false);
 std::thread restart_keys([&] {
  while(!SDL_WasInit(SDL_INIT_VIDEO)){if(done.load())return;SDL_Delay(1);}
  SDL_Delay(100);
  auto key=[](SDL_Keycode sym){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=sym;SDL_PushEvent(&e);};
  key(SDLK_RIGHT);key(SDLK_7);key(SDLK_b);key(SDLK_d);
  for(unsigned i=0;i<5;++i){SDL_Delay(30);key(SDLK_t);}
  SDL_Delay(30);key(SDLK_F2);key(SDLK_TAB);key(SDLK_RIGHT);key(SDLK_7);key(SDLK_b);
  SDL_Delay(100);SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);
 });
 const auto restarted=run_native_viewer(models,view);done.store(true);restart_keys.join();
 if(!restarted)throw std::runtime_error(restarted.detail);
 human_factory=false;
 for(const auto &entity:scene->arena().entities)
  human_factory|=entity.type==26 && entity.owner==1 && entity.grid==ArenaGridPosition{5,1};
 if(!human_factory || scene->simulation().clock()!=1 || scene->simulation().accounts()[1].resource!=340 ||
    scene->simulation().unit_quotas()[1].active!=0 || scene->simulation().base_counter(0)!=2 ||
    scene->simulation().base_counter(1)!=2 || (scene->simulation().ground_deployment()[1] && scene->simulation().ground_deployment()[1]->strength!=0))
  throw std::runtime_error("F2 did not restore skirmish checkpoint, human selection and base cursor");

}
