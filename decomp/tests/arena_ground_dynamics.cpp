#include "comet/arena_ground_dynamics.hpp"
#include <bit>
#include <cassert>
#include <cfenv>
#include <cmath>
#include <limits>
using namespace comet::decomp;
void bits(const std::array<float,4>& v, const std::array<unsigned,4>& expected) {
  for(unsigned i=0;i<4;++i) assert(std::bit_cast<unsigned>(v[i])==expected[i]);
}
int main() {
  ArenaGroundDynamicsQuery q;
  q.steering={1,0,0,0};q.steering_acceleration=4.9f;q.speed_limit=1.6f;q.elapsed=.1f;
  auto r=advance_arena_ground_velocity(q);assert(r);
  bits(r->acceleration,{0x409ccccd,0,0,0});bits(r->velocity,{0x3efae148,0,0,0});
  assert(!r->speed_limited);
  q.velocity={2,3,4,5};q.steering={1,0,-1,0};r=advance_arena_ground_velocity(q);assert(r);
  bits(r->acceleration,{0x409c28f5,0xbcf5c28f,0xc09e147b,0xbd4ccccc});
  bits(r->velocity,{0x3f6d0bee,0x403fced9,0x3fa704ce,0});
  assert(r->speed_limited && std::bit_cast<unsigned>(r->horizontal_speed_squared_before_limit)==0x4193db7f);
  q.velocity={.5f,1,-.25f,0};q.steering={0,0,1,0};q.external_acceleration={1,2,3,4};
  q.steering_acceleration=3.5f;q.speed_limit=.5f;q.elapsed=.05f;q.context_flags=32;q.kind=45;
  r=advance_arena_ground_velocity(q);assert(r);
  bits(r->acceleration,{0x3f7eb851,0x3ffeb851,0x40d0147a,0x40800000});
  bits(r->velocity,{0x3efda488,0x3f8cbc6a,0x3d8aa4e6,0});
  q.kind=47;q.steering_acceleration=3;q.speed_limit=.4f;r=advance_arena_ground_velocity(q);assert(r);
  bits(r->acceleration,{0xbba3d70a,0xbc23d70a,0x404028f5,0});
  bits(r->velocity,{0x3ec8d427,0x3f7fdf3b,0xbda08adc,0});
  q.velocity={2,3,4,-5};q.steering={1,0,-1,0};q.external_acceleration={};
  q.steering_acceleration=4.9f;q.speed_limit=1.6f;q.elapsed=.1f;q.context_flags=0;q.kind=40;
  r=advance_arena_ground_velocity(q);assert(r);
  assert(std::bit_cast<unsigned>(r->velocity[3])==0x80000000u); // masked negative W is -0
  // Zero direction damps rather than instantly stopping existing velocity.
  q={};q.velocity={1,0,0,0};q.steering_acceleration=4.9f;q.speed_limit=1.6f;q.elapsed=.1f;
  r=advance_arena_ground_velocity(q);assert(r && r->velocity[0]<1 && r->velocity[0]>.99f);
  // Do not normalize a zero horizontal vector or limit the vertical lane.
  q.velocity={0,2,0,0};r=advance_arena_ground_velocity(q);assert(r && !r->speed_limited && r->velocity[1]>1.99f);
  const int previous=std::fegetround();std::fesetround(FE_UPWARD);
  r=advance_arena_ground_velocity(q);assert(r && std::fegetround()==FE_UPWARD);std::fesetround(previous);
  for(float dt:{0.f,-1.f,1.f,std::numeric_limits<float>::quiet_NaN()}) {
    q.elapsed=dt;assert(!advance_arena_ground_velocity(q));
  }
  q.elapsed=.1f;q.speed_limit=-1;assert(!advance_arena_ground_velocity(q));
  q.speed_limit=1;q.steering[0]=std::numeric_limits<float>::infinity();assert(!advance_arena_ground_velocity(q));
}
