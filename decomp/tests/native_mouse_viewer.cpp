#define SDL_MAIN_HANDLED
#include "comet/native_viewer.hpp"
#include "comet/native_session.hpp"
#include "comet/native_mouse_input.hpp"
#include <SDL.h>
#include <atomic>
#include <thread>
#include <cmath>
#include <memory>
#include <stdexcept>
using namespace comet::decomp;
int main(){
 ArenaConstructionSimulationConfig c;c.capacity=32;c.seed=4;c.native_extent=8;
 c.players={{{{1,1},0,false},400,400},{{{6,1},1,false},400,400}};
 c.unit_base_available[0]=c.unit_base_available[1]=true;
 c.unit_quotas[0].capacity=c.unit_quotas[1].capacity=2;
 c.base_counters[0]=c.base_counters[1]=100;
 c.production_enabled=true;c.ground_transport_enabled=true;
 c.tuning[26]=ArenaStructureTableValues{100,50,10,20,7.5f};
 NativeSession session;if(!initialize_native_skirmish({},c,session))throw std::runtime_error("mouse arena bootstrap failed");
 auto scene=std::make_unique<NativeConstructionScene>(session.arena,c);
 std::vector<ArenaNativeModel> models;
 for(unsigned slot:{2,3,21,24,37}){
  ArenaNativeModel m{arena_model_asset_manifest()[0],{}};m.spec.original_slot_index=slot;
  m.model.vertices={{{-.4f,0,-.4f},{0,1,0},0,0},{{.4f,0,-.4f},{0,1,0},1,0},{{0,.5f,.4f},{0,1,0},.5f,1}};
  m.model.indices={0,1,2};m.model.bounds_min={-.4f,0,-.4f};m.model.bounds_max={.4f,.5f,.4f};
  NativeSubmesh mesh;mesh.range={0,3,0,2};mesh.material.properties.diffuse={.8f,.4f,.2f,0};m.model.submeshes={mesh};models.push_back(m);
 }
 NativeViewerConfig view;view.width=960;view.height=640;view.hidden=true;view.construction=scene.get();
 auto mouse=[](Uint32 type,int x,int y){SDL_Event e{};e.type=type;e.button.button=SDL_BUTTON_LEFT;e.button.x=x;e.button.y=y;SDL_PushEvent(&e);};
 auto button=[&](unsigned i,bool up=true){const auto r=native_mouse_button(i,view.width,view.height);mouse(SDL_MOUSEBUTTONDOWN,r.x+r.w/2,r.y+r.h/2);if(up)mouse(SDL_MOUSEBUTTONUP,r.x+r.w/2,r.y+r.h/2);};
 auto key=[](SDL_Keycode k){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=k;SDL_PushEvent(&e);};
 // Independently forward-project world cell(2,1) through the initial camera.
 constexpr double rad=3.14159265358979323846/180,dx=-1.5,dz=-2.5;
 const double vx=std::cos(25*rad)*dx+std::sin(25*rad)*dz;
 const double rz=-std::sin(25*rad)*dx+std::cos(25*rad)*dz,vy=-std::sin(55*rad)*rz;
 const double depth=8*.6*3.2-std::cos(55*rad)*rz;
 const int px=int(std::lround((1+vx/depth/std::tan(22.5*rad)/1.5)*480));
 const int py=int(std::lround((1-vy/depth/std::tan(22.5*rad))*320));
 std::atomic<bool> done=false;
 std::thread input([&]{
  while(!SDL_WasInit(SDL_INIT_VIDEO)){if(done.load())return;SDL_Delay(1);}SDL_Delay(100);
  button(6);mouse(SDL_MOUSEBUTTONDOWN,px,py);mouse(SDL_MOUSEBUTTONUP,px,py);SDL_Delay(70);
  // Occupied-cell, HUD and off-board clicks must not charge/build twice.
  mouse(SDL_MOUSEBUTTONDOWN,px,py);mouse(SDL_MOUSEBUTTONUP,px,py);
  SDL_Event move{};move.type=SDL_MOUSEMOTION;move.motion.x=480;move.motion.y=320;SDL_PushEvent(&move);
  button(13);button(12); // Selected factory remains latched while hover moves away.
  mouse(SDL_MOUSEBUTTONDOWN,10,10);mouse(SDL_MOUSEBUTTONUP,10,10);
  mouse(SDL_MOUSEBUTTONDOWN,-100,-100);mouse(SDL_MOUSEBUTTONUP,-100,-100);
  button(8);SDL_Delay(40);button(8);button(10,false);
  for(unsigned i=0;i<20;++i){key(SDLK_t);SDL_Delay(20);}
  SDL_Event lost{};lost.type=SDL_WINDOWEVENT;lost.window.event=SDL_WINDOWEVENT_FOCUS_LOST;SDL_PushEvent(&lost);
  mouse(SDL_MOUSEBUTTONUP,-100,-100);SDL_Delay(40);
  SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);
 });
 const auto rendered=run_native_viewer(models,view);done.store(true);input.join();if(!rendered)throw std::runtime_error(rendered.detail);
 unsigned factories=0;for(const auto& e:scene->arena().entities)if(e.type==26&&e.owner==0&&e.grid==ArenaGridPosition{2,1}){
  ++factories;if(e.production_mode!=1)throw std::runtime_error("moving to the toolbar lost the selected factory");
 }
 if(factories!=1||scene->simulation().accounts()[0].resource!=340)throw std::runtime_error("mouse did not build exactly one selected factory at the projected cell");
 if(scene->simulation().ground_deployment()[0]&&scene->simulation().ground_deployment()[0]->strength!=0)throw std::runtime_error("mouse deployment remained held after focus loss");
 if(scene->simulation().unit_quotas()[0].active==0&&scene->simulation().base_counter(1)==100)throw std::runtime_error("held mouse release never deployed produced units");
 // Mouse restart restores the checkpoint and keeps the toolbar usable.
 scene=std::make_unique<NativeConstructionScene>(session.arena,c);view.construction=scene.get();done.store(false);
 std::thread restart([&]{
  while(!SDL_WasInit(SDL_INIT_VIDEO)){if(done.load())return;SDL_Delay(1);}SDL_Delay(100);
  button(6);mouse(SDL_MOUSEBUTTONDOWN,px,py);mouse(SDL_MOUSEBUTTONUP,px,py);SDL_Delay(70);
  button(9);SDL_Delay(60);button(6);mouse(SDL_MOUSEBUTTONDOWN,px,py);mouse(SDL_MOUSEBUTTONUP,px,py);SDL_Delay(70);
  SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);
 });
 const auto restarted=run_native_viewer(models,view);done.store(true);restart.join();if(!restarted)throw std::runtime_error(restarted.detail);
 if(scene->simulation().accounts()[0].resource!=340||scene->simulation().clock()!=1)throw std::runtime_error("mouse restart failed to restore the launch checkpoint");
}
