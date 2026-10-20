#include "comet/arena_craft_facing.hpp"
#include "fixtures/craft_facing.hpp"
#include <cassert>
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
#include <stdexcept>
using namespace comet::decomp;
int main(){
 for(const auto& f:craft_facing_fixtures){
  ArenaCraftFacingState s;s.yaw=std::bit_cast<float>(f.input[3]);for(unsigned i=0;i<4;++i)s.pending_orientation[i]=std::bit_cast<float>(f.input[4+i]);
  const auto visible=advance_arena_craft_facing(s,std::bit_cast<float>(f.input[0]),std::bit_cast<float>(f.input[1]),std::bit_cast<float>(f.input[2]),f.mode,f.reduced);
  assert(std::bit_cast<unsigned>(s.yaw)==f.output[0]);for(unsigned i=0;i<4;++i){assert(std::bit_cast<unsigned>(s.pending_orientation[i])==f.output[1+i]);assert(std::bit_cast<unsigned>(visible[i])==f.output[5+i]);}
 }

 ArenaCraftFacingState s;auto visible=advance_arena_craft_facing(s,1,0,.1f);
 assert(visible==(std::array<float,4>{0,0,0,1}));
 assert(s.yaw< -1.25f && s.yaw> -1.26f && s.pending_orientation[1]<-.58f);
 const auto pending=s.pending_orientation;visible=advance_arena_craft_facing(s,1,0,.1f);assert(visible==pending && s.yaw< -1.56f);
 auto saved=s;visible=advance_arena_craft_facing(s,0,0,.1f);assert(s.yaw==saved.yaw && s.pending_orientation==saved.pending_orientation && visible==saved.pending_orientation);
 for(float z:{-1.f,1.f}){s={};for(unsigned i=0;i<20;++i)advance_arena_craft_facing(s,0,z,.016f);float n=0;for(float q:s.pending_orientation){assert(std::isfinite(q));n+=q*q;}assert(std::abs(n-1)<.00001f);assert(z<0?std::abs(s.yaw)<.00001f:std::abs(s.yaw)>3.14f);}
 s={};advance_arena_craft_facing(s,.0009f,0,.1f);assert(s.yaw==0);advance_arena_craft_facing(s,.0011f,0,.1f);assert(s.yaw<0);
 s={};advance_arena_craft_facing(s,.001f,0,.1f);assert(s.yaw==0); // strict source threshold
 s={};s.yaw=3.10f;advance_arena_craft_facing(s,1,0,.016f);assert(s.yaw< -2.9f && s.yaw> -3.1416f);
 s={};advance_arena_craft_facing(s,1,0,.25f);assert(s.yaw> -1.571f && s.yaw< -1.570f);
 advance_arena_craft_facing(s,-1,0,0);assert(std::isfinite(s.yaw));
 for(unsigned invalid=0;invalid<7;++invalid){s={};float x=0,z=0,dt=.1f;
  switch(invalid){case 0:x=1.1f;break;case 1:z=std::numeric_limits<float>::quiet_NaN();break;case 2:dt=-.1f;break;case 3:dt=.251f;break;case 4:s.yaw=4;break;case 5:s.pending_orientation={};break;default:s.pending_orientation[0]=std::numeric_limits<float>::infinity();}
  saved=s;bool rejected=false;try{advance_arena_craft_facing(s,x,z,dt);}catch(const std::invalid_argument&){rejected=true;}assert(rejected && s.yaw==saved.yaw && s.pending_orientation==saved.pending_orientation);
 }
 const auto rounding=std::fegetround();std::fesetround(FE_UPWARD);s={};advance_arena_craft_facing(s,1,.3f,.016f);assert(std::fegetround()==FE_UPWARD);std::fesetround(rounding);
}
