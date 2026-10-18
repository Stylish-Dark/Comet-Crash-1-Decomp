#include "comet/arena_weapon_aim.hpp"
#include <cassert>
#include <bit>
#include <cfenv>
#include <cstdint>
#include <cmath>
#include <limits>
using namespace comet::decomp;
int main(){
 ArenaWeaponAimQuery q{{.6f,.48f,.64f,0},0,0,.1f,.05f,-1,1};
 auto r=prepare_arena_weapon_aim(q); assert(r);assert(r->yaw<-.099f&&r->yaw>=-.101f);assert(r->pitch>.049f&&r->pitch<=.051f);assert(std::isfinite(r->remaining_angle));
 q={{0,0,-1,0},0,0,1,1,-1,1};r=prepare_arena_weapon_aim(q);assert(r&&r->yaw==0&&r->pitch==0&&r->remaining_angle<.001f);
 q={{1,0,0,0},0,0,2,2,-1,1};r=prepare_arena_weapon_aim(q);assert(r&&std::fabs(r->yaw+1.5707963f)<.00001f&&r->remaining_angle<.001f);
 q={{0,1,0,0},.2f,0,.1f,.1f,-1,1};r=prepare_arena_weapon_aim(q);assert(r&&std::isfinite(r->yaw)&&r->pitch==0);
 q={{0,0,0,0},.2f,.1f,.1f,.1f,-1,1};r=prepare_arena_weapon_aim(q);assert(r&&r->yaw==q.yaw&&r->pitch==q.pitch);
 // Original instruction-oracle fixture, including a nearly aligned fire angle.
 q={{.8867854475975037f,-.32036784291267395f,.3331306278705597f,0},-2.146193027496338f,-.34550267457962036f,.2492402046918869f,.08733897656202316f,-1,1};
 r=prepare_arena_weapon_aim(q);assert(r);
 const std::array<float,11> actual{r->yaw,r->pitch,r->source_orientation[0],r->source_orientation[1],r->source_orientation[2],r->source_orientation[3],r->muzzle_orientation[0],r->muzzle_orientation[1],r->muzzle_orientation[2],r->muzzle_orientation[3],r->remaining_angle};
 const std::array<std::uint32_t,11> expected{0xbff70f08,0xbea6f8f0,0,0xbf527487,0,0x3f11c15b,0xbdbd4ad1,0xbf4fa9de,0xbe08a8b2,0x3f0fd267,0x3a7ffffe};
 for(unsigned i=0;i<actual.size();i++)assert(std::bit_cast<std::uint32_t>(actual[i])==expected[i]);
 int previous=std::fegetround();std::fesetround(FE_UPWARD);assert(prepare_arena_weapon_aim(q));assert(std::fegetround()==FE_UPWARD);std::fesetround(previous);
 q.direction[0]=std::numeric_limits<float>::quiet_NaN();assert(!prepare_arena_weapon_aim(q));
 q.direction={1,0,0,0};q.yaw_step=-1;assert(!prepare_arena_weapon_aim(q));
}
