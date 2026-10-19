#include "comet/arena_craft_flight.hpp"
#include "fixtures/craft_flight.hpp"
#include <cassert>
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
#include <stdexcept>
using namespace comet::decomp;
int main(){
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
