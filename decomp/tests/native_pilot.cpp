#include "comet/native_pilot.hpp"
#include "comet/native_gamepad_input.hpp"
#include <cassert>
#include <cmath>
#include <limits>
#include <stdexcept>
using namespace comet::decomp;
int main(){
 auto a=arena_player_stick(.3f,.4f);assert(a.x==.3f && a.z==.4f && a.magnitude==.5f);
 a=arena_player_stick(1,1);assert(std::abs(a.x-.70710678f)<1e-6f && a.x==a.z && a.magnitude>1);
 for(unsigned x=0;x<256;++x)for(unsigned z=0;z<256;++z){
  const float ax=decode_arena_pad_axis(x),az=decode_arena_pad_axis(z);a=arena_player_stick(ax,az);
  assert(std::hypot(a.x,a.z)<=1.000001f);
  if(std::hypot(ax,az)<=1)assert(a.x==ax && a.z==az);
 }
 NativePilot p;p.reset({1,1},24);assert(p.cursor()==(ArenaGridPosition{1,1}));
 p.advance(1,0,.1f,true);assert(p.position().x==1.5f); // old-velocity position order
 p.advance(1,0,.1f,true);assert(p.position().x>1.95f && p.position().x<1.97f);
 const auto before=p.position();p.advance(1,1,.1f,false);assert(p.position().x==before.x);
 p.advance(1,1,.1f,true);assert(p.position().x==before.x); // disabled input cleared velocity
 p.advance(1,1,.1f,true);assert(p.position().x>before.x && p.position().z>before.z);
 float norm=0;for(float v:p.orientation())norm+=v*v;assert(std::abs(norm-1)<1e-6f);
 for(unsigned i=0;i<100;++i)p.advance(-1,-1,.25f,true);
 assert(p.position().x>.239f && p.position().x<.25f && p.position().z>.239f && p.position().z<.25f);
 for(unsigned i=0;i<100;++i)p.advance(1,1,.25f,true);
 assert(p.position().x<23.761f && p.position().x>23.75f && p.position().z<23.761f && p.position().z>23.75f); // original epsilon correction, not a clamp
 const auto edge=p.position();
 bool rejected=false;try{p.advance(0,0,std::numeric_limits<float>::quiet_NaN(),true);}catch(const std::invalid_argument&){rejected=true;}
 assert(rejected && p.position().x==edge.x);
 rejected=false;try{p.reset({24,0},24);}catch(const std::invalid_argument&){rejected=true;}
 assert(rejected && p.position().x==edge.x);
 rejected=false;try{arena_player_stick(1.01f,0);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);
 p.reset({2,3},24);assert(p.position().x==2.5f && p.position().z==3.5f && p.orientation()[3]==1);
 // Degenerate one-cell arena remains navigable without invalid cursor cells.
 p.reset({0,0},1);for(unsigned i=0;i<100;++i)p.advance(1,1,.25f,true);
 assert(p.cursor()==(ArenaGridPosition{0,0}));
 p.reset({0,0},2);for(unsigned i=0;i<100;++i)p.advance(1,0,.1f,true);
 const float east=p.position().x;for(unsigned i=0;i<10;++i)p.advance(-1,0,.1f,true);
 assert(p.position().x<east && p.cursor().x<2);
 NativeGamepadInput pad;NativeGamepadSample s;s.connected=true;pad.advance(s,.01f,true);s.x=32767;
 auto action=pad.advance(s,.01f,true);assert(action.move_x>.9f && action.move_z==0);
 pad.cancel();assert(pad.advance(s,.01f,true).move_x==0);
}
