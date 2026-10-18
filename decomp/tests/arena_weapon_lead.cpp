#include "comet/arena_weapon_lead.hpp"
#include <cassert>
#include <bit>
#include <cfenv>
#include <cstdint>
#include <cmath>
#include <limits>
using namespace comet::decomp;
int main(){
 auto r=predict_arena_weapon_target({0,0,0,0},{10,0,0,0},{0,1,0,0},5);assert(r&&(*r)[1]>2&&(*r)[1]<2.05f&&(*r)[0]==10);
 assert(std::bit_cast<std::uint32_t>((*r)[1])==0x4002a3b3u);
 int previous=std::fegetround();std::fesetround(FE_UPWARD);assert(predict_arena_weapon_target({0,0,0,0},{10,0,0,0},{0,1,0,0},5));assert(std::fegetround()==FE_UPWARD);std::fesetround(previous);
 r=predict_arena_weapon_target({0,0,0,0},{10,0,0,0},{0,0,0,0},5);assert(r&&(*r)[0]==10&&(*r)[1]==0);
 r=predict_arena_weapon_target({0,0,0,0},{10,0,0,0},{0,10,0,0},5);assert(r&&(*r)[0]==10&&(*r)[1]==0);
 r=predict_arena_weapon_target({0,0,0,0},{10,0,0,0},{5,0,0,0},5);assert(r&&(*r)[0]==10);
 assert(!predict_arena_weapon_target({0,0,0,0},{10,0,0,0},{0,1,0,0},0));
 assert(!predict_arena_weapon_target({0,0,0,1},{10,0,0,0},{0,1,0,0},5));
 assert(!predict_arena_weapon_target({0,0,0,0},{10,0,0,0},{0,std::numeric_limits<float>::infinity(),0,0},5));
}
