#include "comet/arena_ground_orientation.hpp"
#include <cassert>
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
using namespace comet::decomp;
int main(){
 ArenaGroundOrientationQuery q;
 assert(prepare_arena_ground_orientation(q)==q.quaternion);
 q.velocity={1,1,0,0};q.prelimit_speed_squared=2;q.elapsed=.01f;q.bank_scale=.5f;
 auto r=prepare_arena_ground_orientation(q);assert(r);assert((*r)[0]>0&&(*r)[1]<0&&(*r)[3]>.9f);
 q.velocity={1,0,0,0};q.bank_scale=0;r=prepare_arena_ground_orientation(q);assert(r&&(*r)[1]<0);
 float length=0;for(float x:*r)length+=x*x;assert(std::fabs(length-1)<1e-6f);
 q.prelimit_speed_squared=std::bit_cast<float>(0x3e4cccccu);q.steering_direction={-1,0,0,0};r=prepare_arena_ground_orientation(q);assert(r&&(*r)[1]>0);
 q.prelimit_speed_squared=2;q.velocity={1,1,0,0};q.bank_scale=.5f;q.elapsed=.2f/10.995573043823242f;
 r=prepare_arena_ground_orientation(q);assert(r);
 const std::array<float,4> golden{.08735190331935883f,-.13073134422302246f,.030673902481794357f,.9870856404304504f};
 for(unsigned i=0;i<4;i++)assert(std::fabs((*r)[i]-golden[i])<2e-7f);
 int old=std::fegetround();std::fesetround(FE_UPWARD);assert(prepare_arena_ground_orientation(q));assert(std::fegetround()==FE_UPWARD);std::fesetround(old);
 q.kind=46;assert(!prepare_arena_ground_orientation(q));q.kind=40;
 q.quaternion={0,0,0,2};assert(!prepare_arena_ground_orientation(q));q.quaternion={0,0,0,1};
 q.elapsed=.251f;assert(!prepare_arena_ground_orientation(q));
 q.elapsed=std::numeric_limits<float>::infinity();assert(!prepare_arena_ground_orientation(q));
}
