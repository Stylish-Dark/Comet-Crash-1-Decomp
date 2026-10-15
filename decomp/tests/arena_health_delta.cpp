#include "comet/arena_health_delta.hpp"
#include <cassert>
#include <bit>
#include <cfenv>
#include <limits>
using namespace comet::decomp;
namespace {
void put(ArenaEntitySlot&s,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)s[o+i]=v>>(24-8*i);}
void fp(ArenaEntitySlot&s,unsigned o,float v){put(s,o,std::bit_cast<unsigned>(v));}
unsigned word(const ArenaEntitySlot&s,unsigned o){unsigned v=0;for(unsigned i=0;i<4;++i)v=(v<<8)|s[o+i];return v;}
ArenaEntitySlot slot(){ArenaEntitySlot s;for(unsigned i=0;i<s.size();++i)s[i]=i*37u;fp(s,0x38,25);fp(s,0x64,100);put(s,0x60,0);return s;}
void unchanged_except_health(const ArenaEntitySlot&a,const ArenaEntitySlot&b){for(unsigned i=0;i<a.size();++i)if(i<0x38 || i>=0x3c)assert(a[i]==b[i]);}
}
int main(){
 {float sum=-5;assert(accumulate_arena_health_delta(sum,-7));assert(sum==-12);assert(accumulate_arena_health_delta(sum,20));assert(sum==8);}
 {float sum=std::numeric_limits<float>::max();assert(!accumulate_arena_health_delta(sum,sum));assert(sum==std::numeric_limits<float>::max());}
 {float sum=3;assert(!accumulate_arena_health_delta(sum,std::numeric_limits<float>::quiet_NaN()));assert(sum==3);}
 {auto s=slot();const auto before=s;assert(apply_arena_health_delta(s,-30)==ArenaHealthDelta::Applied);assert(word(s,0x38)==std::bit_cast<unsigned>(-5.f));unchanged_except_health(before,s);}
 {auto s=slot();assert(apply_arena_health_delta(s,100)==ArenaHealthDelta::Applied);assert(word(s,0x38)==std::bit_cast<unsigned>(100.f));}
 {auto s=slot();assert(apply_arena_health_delta(s,5)==ArenaHealthDelta::Applied);assert(word(s,0x38)==std::bit_cast<unsigned>(30.f));}
 {auto s=slot();fp(s,0x38,110);assert(apply_arena_health_delta(s,0)==ArenaHealthDelta::Applied);assert(word(s,0x38)==std::bit_cast<unsigned>(100.f));}
 {auto s=slot();put(s,0x60,0x00100000);const auto before=s;assert(apply_arena_health_delta(s,-100)==ArenaHealthDelta::Suppressed);assert(s==before);}
 {auto s=slot();put(s,0x60,0xffefffff);assert(apply_arena_health_delta(s,-10)==ArenaHealthDelta::Applied);assert(word(s,0x38)==std::bit_cast<unsigned>(15.f));}
 {const int previous=std::fegetround();assert(std::fesetround(FE_UPWARD)==0);auto s=slot();fp(s,0x38,1);assert(apply_arena_health_delta(s,0x1p-24f)==ArenaHealthDelta::Applied);assert(word(s,0x38)==0x3f800000);assert(std::fegetround()==FE_UPWARD);assert(std::fesetround(previous)==0);}
 for(float bad:{std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::denorm_min()}){
  auto s=slot();const auto before=s;assert(apply_arena_health_delta(s,bad)==ArenaHealthDelta::Invalid);assert(s==before);
  for(unsigned offset:{0x38u,0x64u}){s=slot();fp(s,offset,bad);const auto malformed=s;assert(apply_arena_health_delta(s,1)==ArenaHealthDelta::Invalid);assert(s==malformed);}
 }
 {auto s=slot();fp(s,0x38,std::numeric_limits<float>::max());fp(s,0x64,std::numeric_limits<float>::max());assert(apply_arena_health_delta(s,std::numeric_limits<float>::max())==ArenaHealthDelta::Applied);assert(word(s,0x38)==0x7f7fffff);}
}
