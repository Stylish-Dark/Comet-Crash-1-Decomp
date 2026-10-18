#include "comet/arena_impulse.hpp"
#include <bit>
#include <cassert>
#include <cfenv>
#include <limits>
using namespace comet::decomp;
namespace {
void word(ArenaEntitySlot&s,unsigned o,unsigned v){for(unsigned i=0;i<4;i++)s[o+i]=v>>(24-8*i);}
void scalar(ArenaEntitySlot&s,unsigned o,float f){word(s,o,std::bit_cast<unsigned>(f));}
float scalar(const ArenaEntitySlot&s,unsigned o){unsigned w=0;for(unsigned i=0;i<4;i++)w=(w<<8)|s[o+i];return std::bit_cast<float>(w);}
}
int main(){
 ArenaEntitySlot s{};word(s,0x60,0x6000);scalar(s,0x58,10);s[0x7d]=100;scalar(s,0x40,1);word(s,0x4c,0x12345678);
 assert(apply_arena_impulse(s,{4,0,0,0})==ArenaImpulseDelta::Applied);
 assert(scalar(s,0x40)>1.39999f && scalar(s,0x40)<1.40001f && s[0x4c]==0x12);
 auto clamped=s;scalar(clamped,0x40,0);assert(apply_arena_impulse(clamped,{80,0,0,0})==ArenaImpulseDelta::Applied);
 assert(scalar(clamped,0x40)>.79999f && scalar(clamped,0x40)<.80001f);
 auto before=s;word(s,0x60,0);before=s;
 assert(apply_arena_impulse(s,{4,0,0,0})==ArenaImpulseDelta::Suppressed && s==before);
 word(s,0x60,0x4000);scalar(s,0x58,0);before=s;
 assert(apply_arena_impulse(s,{4,0,0,0})==ArenaImpulseDelta::Invalid && s==before);
 std::array<float,4> pending{1,2,3,0};auto old=pending;
 assert(!accumulate_arena_impulse(pending,{0,std::numeric_limits<float>::quiet_NaN(),0,0}) && pending==old);
 assert(accumulate_arena_impulse(pending,{1,-1,0,0}) && pending[0]==2 && pending[1]==1);
 const int rounding=std::fegetround();std::fesetround(FE_UPWARD);word(s,0x60,0x4000);scalar(s,0x58,10);
 assert(apply_arena_impulse(s,{0,0,0,0})==ArenaImpulseDelta::Applied && std::fegetround()==FE_UPWARD);std::fesetround(rounding);
}
