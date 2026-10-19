#define SDL_MAIN_HANDLED
#include "comet/native_viewer.hpp"
#include "comet/native_session.hpp"
#include <SDL.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>
using namespace comet::decomp;
int main(){
 ArenaConstructionSimulationConfig c;c.capacity=32;c.seed=4;
 c.players={{{{1,1},0,false},400,400},{{{22,22},1,false},400,400}};
 c.unit_base_available[0]=c.unit_base_available[1]=true;
 c.unit_quotas[0].capacity=c.unit_quotas[1].capacity=2;
 c.base_counters[0]=c.base_counters[1]=100;c.native_opponents[1]=true;
 c.production_enabled=c.ground_transport_enabled=true;
 c.tuning[26]=ArenaStructureTableValues{100,50,10,20,7.5f};
 NativeSession session;if(!initialize_native_skirmish({},c,session))throw std::runtime_error("skirmish setup");
 std::vector<ArenaNativeModel> models;
 for(unsigned slot:{0,2,3,21,24,37}){
  ArenaNativeModel m{arena_model_asset_manifest()[0],{}};m.spec.original_slot_index=slot;
  m.model.vertices={{{-.4f,0,-.4f},{0,1,0},0,0},{{.4f,0,-.4f},{0,1,0},1,0},{{0,.5f,.4f},{0,1,0},.5f,1}};
  m.model.indices={0,1,2};m.model.bounds_min={-.4f,0,-.4f};m.model.bounds_max={.4f,.5f,.4f};
  NativeSubmesh mesh;mesh.range={0,3,0,2};mesh.material.properties.diffuse={.8f,.4f,.2f,0};m.model.submeshes={mesh};models.push_back(m);
 }
 for(unsigned phase=0;phase<5;++phase){
  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
  auto scene=std::make_unique<NativeConstructionScene>(session.arena,c);
  NativeViewerConfig view;view.width=320;view.height=240;view.hidden=true;view.construction=scene.get();
  std::atomic<bool> done=false,controller_ok=false;
  std::thread controller([&]{
   const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
   while(!SDL_WasInit(SDL_INIT_GAMECONTROLLER)){
    if(done.load())return;
    if(std::chrono::steady_clock::now()>deadline){SDL_Event q{};q.type=SDL_QUIT;SDL_PushEvent(&q);return;}
    SDL_Delay(1);
   }
   SDL_LockJoysticks();
   const int device=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);
   SDL_Joystick *joy=device<0?nullptr:SDL_JoystickOpen(device);
   if(joy){SDL_JoystickSetVirtualAxis(joy,SDL_CONTROLLER_AXIS_TRIGGERLEFT,-32768);SDL_JoystickSetVirtualAxis(joy,SDL_CONTROLLER_AXIS_TRIGGERRIGHT,-32768);}
   SDL_UnlockJoysticks();
   if(!joy){SDL_Event q{};q.type=SDL_QUIT;SDL_PushEvent(&q);return;}
   controller_ok.store(SDL_IsGameController(device));SDL_Delay(200);
   auto button=[&](SDL_GameControllerButton b,bool held){SDL_JoystickSetVirtualButton(joy,b,held);SDL_Delay(100);};
   auto tap=[&](SDL_GameControllerButton b){button(b,true);button(b,false);};
   for(unsigned i=0;i<6;++i)tap(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
   if(phase>=3){
    SDL_Event mode{};mode.type=SDL_KEYDOWN;mode.key.keysym.sym=SDLK_F3;SDL_PushEvent(&mode);SDL_Delay(100);
    tap(SDL_CONTROLLER_BUTTON_START);
    // Move until the ship reaches the east edge; avoids timing-dependent cell selection.
    SDL_JoystickSetVirtualAxis(joy,SDL_CONTROLLER_AXIS_LEFTX,32767);SDL_Delay(4500);
    SDL_JoystickSetVirtualAxis(joy,SDL_CONTROLLER_AXIS_LEFTX,0);SDL_Delay(100);
    tap(SDL_CONTROLLER_BUTTON_START);
    SDL_JoystickSetVirtualAxis(joy,SDL_CONTROLLER_AXIS_LEFTX,-32768);SDL_Delay(400);
    SDL_Event key{};key.type=SDL_KEYDOWN;key.key.keysym.sym=phase==3?SDLK_LEFT:SDLK_F2;SDL_PushEvent(&key);SDL_Delay(100);
    SDL_JoystickSetVirtualAxis(joy,SDL_CONTROLLER_AXIS_LEFTX,0);SDL_Delay(100);
    tap(SDL_CONTROLLER_BUTTON_A);
    SDL_JoystickClose(joy);SDL_JoystickDetachVirtual(device);SDL_Delay(100);
    SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);return;
   }
   SDL_JoystickSetVirtualAxis(joy,SDL_CONTROLLER_AXIS_LEFTX,32767);SDL_Delay(100);
   SDL_JoystickSetVirtualAxis(joy,SDL_CONTROLLER_AXIS_LEFTX,0);SDL_Delay(100);
   tap(SDL_CONTROLLER_BUTTON_A);
   if(phase!=1){tap(SDL_CONTROLLER_BUTTON_Y);tap(SDL_CONTROLLER_BUTTON_LEFTSTICK);
    SDL_JoystickSetVirtualAxis(joy,SDL_CONTROLLER_AXIS_TRIGGERRIGHT,32767);SDL_Delay(100);
   }else button(SDL_CONTROLLER_BUTTON_X,true);
   for(unsigned i=0;i<8;++i){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_t;SDL_PushEvent(&e);SDL_Delay(100);}
   bool detached=false;
   if(phase==1)tap(SDL_CONTROLLER_BUTTON_BACK);
   else {
    // Freeze travel so quota changes expose release surviving an interruption.
    SDL_Event motion{};motion.type=SDL_KEYDOWN;motion.key.keysym.sym=SDLK_m;SDL_PushEvent(&motion);SDL_Delay(100);
    if(phase==0){
     SDL_Event focus{};focus.type=SDL_WINDOWEVENT;focus.window.event=SDL_WINDOWEVENT_FOCUS_LOST;SDL_PushEvent(&focus);SDL_Delay(100);
     focus.window.event=SDL_WINDOWEVENT_FOCUS_GAINED;SDL_PushEvent(&focus);SDL_Delay(100);
    }else {SDL_JoystickClose(joy);SDL_JoystickDetachVirtual(device);detached=true;SDL_Delay(100);}
    // R2 remains held through focus regain. More production must stay pending.
    for(unsigned i=0;i<6;++i){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_t;SDL_PushEvent(&e);SDL_Delay(100);}
   }
   if(!detached){SDL_JoystickClose(joy);SDL_JoystickDetachVirtual(device);SDL_Delay(100);}
   SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);
  });
  const auto result=run_native_viewer(models,view);done.store(true);controller.join();
  if(!result || !controller_ok.load())throw std::runtime_error("virtual gamepad/render startup failed: "+result.detail);
  if(phase==3){
   bool built=false;for(const auto&e:scene->arena().entities)built|=e.type==26&&e.owner==0&&e.grid==ArenaGridPosition{23,1};
   if(!built || scene->simulation().accounts()[0].resource!=340)throw std::runtime_error("pilot did not move build cursor to east edge");
  }else if(phase==4){
   if(scene->simulation().clock()!=0 || scene->simulation().accounts()[0].resource!=400 || scene->arena().entities.size()!=2)
    throw std::runtime_error("pilot restart did not restore occupied base cursor");
  }else if(phase!=1){
   bool built=false;for(const auto&e:scene->arena().entities)built|=e.type==26&&e.owner==0&&e.grid==ArenaGridPosition{2,1} && e.production_mode==1;
   if(!built || scene->simulation().accounts()[0].resource!=340 || scene->simulation().unit_quotas()[0].active!=1 || scene->simulation().unit_quotas()[0].pending!=1)
    throw std::runtime_error("gamepad did not navigate/build/release");
  }else if(scene->simulation().clock()!=0 || scene->simulation().accounts()[0].resource!=400 || scene->arena().entities.size()!=2)
   throw std::runtime_error("controller restart failed: clock="+std::to_string(scene->simulation().clock())+" funds="+std::to_string(scene->simulation().accounts()[0].resource)+" entities="+std::to_string(scene->arena().entities.size()));
  const auto control=scene->simulation().ground_deployment()[0];
  if(control && control->strength!=0)throw std::runtime_error("gamepad release remained held after interruption");
 }
}
