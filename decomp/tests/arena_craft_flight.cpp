#include "comet/arena_craft_flight.hpp"
#include "fixtures/craft_flight.hpp"
#include "fixtures/craft_bounds.hpp"
#include <cassert>
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
#include <stdexcept>
using namespace comet::decomp;
int main(){
 for(const auto& f:craft_bounds_fixtures){
  ArenaCraftFlightState state;for(unsigned i=0;i<4;++i){state.position[i]=std::bit_cast<float>(f.input[i]);state.velocity[i]=std::bit_cast<float>(f.input[4+i]);}
  const auto& a=f.bounds;ArenaCraftBounds b{a[0],a[1],std::bit_cast<float>(a[2]),std::bit_cast<float>(a[3]),static_cast<std::uint8_t>(a[4]),bool(a[5]&0x4000),bool(a[5]&0x02000000)};
  const auto contact=advance_arena_craft_in_arena(state,std::bit_cast<float>(f.input[8]),std::bit_cast<float>(f.input[9]),std::bit_cast<float>(f.input[10]),b,f.mode,f.slow);
  for(unsigned i=0;i<4;++i){
   assert(std::bit_cast<unsigned>(state.position[i])==f.output[i]);
   assert(std::bit_cast<unsigned>(state.velocity[i])==f.output[4+i]);
   assert(std::bit_cast<unsigned>(contact.accumulated_impulse[i])==f.output[8+i]);
  }
  assert((contact.contacts>0)==bool(f.output[12]));
 }
 ArenaCraftBounds bounds;
 // A post-response clamp leaves roughly 5.4 outward speed here. Original
 // containment acts on old velocity, so neutral response starts near zero.
 ArenaCraftFlightState wall{{23.4f,.85f,12,0},{10,0,0,0}};
 auto hit=advance_arena_craft_in_arena(wall,0,0,.1f,bounds);
 assert(hit.contacts==1 && hit.energetic_impact && hit.accumulated_impulse[0]<-9.99f);
 assert(wall.position[0]<23.5f && wall.position[0]>23.49f && std::abs(wall.velocity[0])<.00001f);
 advance_arena_craft_in_arena(wall,-1,0,.1f,bounds);
 advance_arena_craft_in_arena(wall,-1,0,.1f,bounds);assert(wall.position[0]<23.1f);
 for(unsigned axis=0;axis<3;++axis)for(bool upper:{false,true}){
  ArenaCraftFlightState plane{{12,1,12,0},{}};
  plane.position[axis]=upper?(axis==1?2.1f:24.1f):-.1f;plane.velocity[axis]=upper?3.f:-3.f;
  hit=advance_arena_craft_in_arena(plane,0,0,0,bounds);
  assert(hit.contacts==1 && hit.energetic_impact);
  assert(plane.position[axis]>(upper?(axis==1?1.49f:23.49f):.5f));
  assert(plane.position[axis]<(upper?(axis==1?1.5f:23.5f):.51f));
 }
 bounds.radius=.2f;bounds.lateral_radius=.4f;
 ArenaCraftFlightState floor{{12,.3f,12,0},{}};
 hit=advance_arena_craft_in_arena(floor,0,0,0,bounds);assert(hit.contacts==0);
 bounds.floor_uses_lateral_radius=true;
 hit=advance_arena_craft_in_arena(floor,0,0,0,bounds);assert(hit.contacts==1 && floor.position[1]>.4f);
 bounds={};
 for(unsigned invalid=0;invalid<6;++invalid){
  ArenaCraftFlightState state{{1,1,1,0},{1,0,0,0}};const auto saved=state;auto b=bounds;
  switch(invalid){case 0:b.width=0;break;case 1:b.depth=25;break;case 2:b.radius=1.1f;break;case 3:b.lateral_radius=13;break;case 4:b.radius=std::numeric_limits<float>::quiet_NaN();break;default:b.lateral_radius=-1;}
  bool rejected=false;const auto previous=std::fegetround();std::fesetround(FE_UPWARD);
  try{advance_arena_craft_in_arena(state,1,0,.1f,b);}catch(const std::invalid_argument&){rejected=true;}
  assert(rejected && state.position==saved.position && state.velocity==saved.velocity && std::fegetround()==FE_UPWARD);std::fesetround(previous);
 }
 ArenaCraftFlightState overflow{{1,1,1,1000000},{0,0,0,1000000}};const auto saved=overflow;
 bool overflow_rejected=false;try{advance_arena_craft_in_arena(overflow,0,0,.25f,bounds);}catch(const std::invalid_argument&){overflow_rejected=true;}
 assert(overflow_rejected && overflow.position==saved.position && overflow.velocity==saved.velocity);
 const auto prior=std::fegetround();std::fesetround(FE_DOWNWARD);
 advance_arena_craft_in_arena(wall,0,0,.01f,bounds);assert(std::fegetround()==FE_DOWNWARD);std::fesetround(prior);

 for(const auto& f:craft_flight_fixtures){
  ArenaCraftFlightState state;
  for(unsigned i=0;i<4;++i){state.position[i]=std::bit_cast<float>(f.input[i]);state.velocity[i]=std::bit_cast<float>(f.input[4+i]);}
  advance_arena_craft_flight(state,std::bit_cast<float>(f.input[8]),std::bit_cast<float>(f.input[9]),std::bit_cast<float>(f.input[10]),f.mode,f.reduced);
  for(unsigned i=0;i<4;++i){assert(std::bit_cast<unsigned>(state.position[i])==f.output[i]);assert(std::bit_cast<unsigned>(state.velocity[i])==f.output[4+i]);}
 }

 ArenaCraftFlightState s{{1.5f,.85f,1.5f,0},{}};
 advance_arena_craft_flight(s,1,0,.016f,true,false);
 assert(s.position[0]==1.5f && std::bit_cast<unsigned>(s.velocity[0])==0x3f3c6a7fu);
 ArenaCraftFlightState fast{{1.5f,.85f,1.5f,0},{}};
 advance_arena_craft_flight(fast,1,0,.016f,false,false);
 assert(fast.position[0]==1.5f && fast.velocity[0]==40);
 auto p=s;advance_arena_craft_flight(s,0,0,.016f);
 assert(s.position[0]>p.position[0] && s.velocity[0]>0 && s.velocity[0]<p.velocity[0]);
 p=s;bool rejected=false;
 try{advance_arena_craft_flight(s,0,0,std::numeric_limits<float>::quiet_NaN());}catch(const std::invalid_argument&){rejected=true;}
 assert(rejected && s.position==p.position && s.velocity==p.velocity);
 s={{1000000,0,0,0},{1000000,0,0,0}};p=s;rejected=false;
 try{advance_arena_craft_flight(s,0,0,.25f);}catch(const std::invalid_argument&){rejected=true;}
 assert(rejected && s.position==p.position && s.velocity==p.velocity);
 s={{},{std::numeric_limits<float>::denorm_min(),0,0,0}};
 advance_arena_craft_flight(s,0,0,.125f,false);assert(s.position[0]==0 && s.velocity[0]==0);
 auto rounding=std::fegetround();std::fesetround(FE_UPWARD);
 advance_arena_craft_flight(s,0,0,.016f);assert(std::fegetround()==FE_UPWARD);std::fesetround(rounding);
 ArenaCraftFlightState slow{},normal{};
 advance_arena_craft_flight(slow,1,0,.1f,true,true);advance_arena_craft_flight(normal,1,0,.1f,true,false);
 assert(slow.velocity[0]<normal.velocity[0]);
 // Four-lane position integration precedes forcing the new Y velocity to zero.
 s={{0,0,0,0},{1,2,3,4}};advance_arena_craft_flight(s,0,0,.125f,false);
 assert(s.position==(std::array<float,4>{.125f,.25f,.375f,.5f}) && s.velocity[1]==0);
}
