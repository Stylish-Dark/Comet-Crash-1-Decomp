#include "comet/arena_ground_height.hpp"
#include <cassert>
#include <bit>
#include <cfenv>
#include <limits>
using namespace comet::decomp;
int main(){
 ArenaGroundHeightQuery q;q.position={3,.2f,3,0};q.velocity={1,.1f,0,0};q.direction={1,0,0,0};q.enabled=true;
 auto r=steer_arena_ground_height(q);assert(r && (*r)[1]>.9f && (*r)[0]<.4f);
 assert(std::bit_cast<unsigned>((*r)[0])==0x3eac344a && std::bit_cast<unsigned>((*r)[1])==0x3f7115ff);
 const int previous=std::fegetround();std::fesetround(FE_UPWARD);
 assert(steer_arena_ground_height(q));assert(std::fegetround()==FE_UPWARD);std::fesetround(previous);
 q.enabled=false;r=steer_arena_ground_height(q);assert(r && *r==q.direction);
 q.enabled=true;q.position[1]=1;q.velocity[1]=0;r=steer_arena_ground_height(q);assert(r && (*r)[0]==1 && (*r)[1]==0);
 q.position[1]=1.2f;q.velocity[1]=.1f;r=steer_arena_ground_height(q);assert(r && (*r)[1]<0);
 q.direction={};q.position[1]=1;q.velocity[1]=0;assert(!steer_arena_ground_height(q));
 q.position[1]=std::numeric_limits<float>::infinity();assert(!steer_arena_ground_height(q));
}
