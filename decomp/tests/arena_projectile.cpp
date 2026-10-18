#include "comet/arena_projectile.hpp"
#include <cassert>
#include <bit>
#include <cfenv>
#include <limits>
using namespace comet::decomp;
namespace {
void put(ArenaEntitySlot&s,unsigned o,unsigned v){for(unsigned i=0;i<4;i++)s[o+i]=v>>(24-8*i);}
unsigned word(const ArenaEntitySlot&s,unsigned o){unsigned v=0;for(unsigned i=0;i<4;i++)v=(v<<8)|s[o+i];return v;}
void fp(ArenaEntitySlot&s,unsigned o,float f){put(s,o,std::bit_cast<unsigned>(f));}
ArenaEntitySlot fixture(){ArenaEntitySlot s{};s.fill(0xa5);s[15]=15;for(unsigned o:{0u,4u,8u,0x40u,0x44u,0x48u})fp(s,o,0);fp(s,0,1);fp(s,0x40,4);fp(s,0x58,3);put(s,0x60,0x1234567f);return s;}
}
int main(){
 {auto s=fixture(),expected=s;fp(expected,0,2);put(expected,0x60,0x1234567d);auto r=advance_arena_projectile(s,.25f);assert(r.status==ArenaProjectileStatus::Active);assert(s==expected);assert(!r.impact_message);}
 {auto s=fixture(),expected=s;put(expected,0x60,0x1234567d);auto r=advance_arena_projectile(s,.25f,{true,6,123,{},{},{}});assert(r.status==ArenaProjectileStatus::Retire);assert(s==expected);assert(r.impact_message&&r.target_index==123);assert(std::bit_cast<unsigned>(r.health_delta)==0x80000000);assert(r.impulse[0]==12&&r.impulse[1]==0);}
 {auto s=fixture();s[15]=3;fp(s,4,2);put(s,0x80,9);auto r=advance_arena_projectile(s,.25f,{},false,{0,-4,0,0});assert(r.status==ArenaProjectileStatus::Active);assert(word(s,0x80)==10);assert(word(s,0x44)==0xbf800000);assert(word(s,4)==0x3fe00000);}
 {auto s=fixture();s[15]=3;fp(s,4,1);auto r=advance_arena_projectile(s,.25f,{true,5,123,{},{},{}});assert(r.status==ArenaProjectileStatus::Retire&&r.impact_message&&r.target_index==123);assert(r.impulse[0]==12);assert(word(s,0)==0x3f800000);}
 for(unsigned type:{0u,1u,5u,7u}){auto s=fixture();auto r=advance_arena_projectile(s,.1f,{true,type,42,{},{},{}});assert(r.status==ArenaProjectileStatus::Retire&&!r.impact_message);}
 {auto s=fixture();assert(advance_arena_projectile(s,.1f,{},true).status==ArenaProjectileStatus::Retire);}
 for(unsigned mode=0;mode<8;mode++){auto s=fixture();float dt=.1f;ArenaProjectileContact c{};if(mode==0)dt=0;if(mode==1)dt=.26f;if(mode==2)dt=std::numeric_limits<float>::denorm_min();if(mode==3)fp(s,0,65);if(mode==4)fp(s,0x40,1025);if(mode==5){fp(s,0x58,std::numeric_limits<float>::infinity());c={true,6,42,{},{},{}};}if(mode==6)c={true,8,0,{},{},{}};if(mode==7){fp(s,0,0);fp(s,0x40,std::numeric_limits<float>::min());dt=.1f;}auto before=s;assert(advance_arena_projectile(s,dt,c).status==ArenaProjectileStatus::Invalid);assert(s==before);}
 {auto s=fixture();s[15]=2;auto before=s;assert(advance_arena_projectile(s,.1f).status==ArenaProjectileStatus::Unsupported&&s==before);}
 {auto s=fixture();std::fesetround(FE_UPWARD);advance_arena_projectile(s,.1f);assert(std::fegetround()==FE_UPWARD);std::fesetround(FE_TONEAREST);}
}
