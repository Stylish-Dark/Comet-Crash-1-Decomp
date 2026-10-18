#include "comet/arena_impulse.hpp"
#include "comet/arena_health_delta.hpp"
#include "arena_spu_math.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
unsigned word(const ArenaEntitySlot&s,unsigned o){unsigned w=0;for(unsigned i=0;i<4;i++)w=(w<<8)|s[o+i];return w;}
float scalar(const ArenaEntitySlot&s,unsigned o){return std::bit_cast<float>(word(s,o));}
void scalar(ArenaEntitySlot&s,unsigned o,float f){const auto w=std::bit_cast<unsigned>(f);for(unsigned i=0;i<4;i++)s[o+i]=w>>(24-8*i);}
bool number(float f){return std::isfinite(f)&&std::fpclassify(f)!=FP_SUBNORMAL;}
float mul(float a,float b){volatile float f=a*b;return f;}
struct Rounding{int previous=std::fegetround();Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO))throw std::runtime_error("impulse rounding unavailable");}~Rounding(){std::fesetround(previous);}};
}
bool accumulate_arena_impulse(std::array<float,4>&pending,const std::array<float,4>&delta){
 if(pending[3]!=0 || delta[3]!=0)return false;
 auto next=pending;for(unsigned i=0;i<3;i++)if(!accumulate_arena_health_delta(next[i],delta[i]) || std::abs(next[i])>1e6f)return false;
 pending=next;return true;
}
ArenaImpulseDelta apply_arena_impulse(ArenaEntitySlot&s,const std::array<float,4>&input){
 if(input[3]!=0)return ArenaImpulseDelta::Invalid;
 for(unsigned i=0;i<3;i++)if(!number(input[i])||std::abs(input[i])>1e6f || !number(scalar(s,0x40+4*i)))return ArenaImpulseDelta::Invalid;
 if(!(word(s,0x60)&0x4000))return ArenaImpulseDelta::Suppressed;
 const float mass=scalar(s,0x58);if(!number(mass)||mass<=0||mass>1e6f)return ArenaImpulseDelta::Invalid;
 Rounding rounding;auto impulse=input;
 const float sq=std::fma(input[2],input[2],std::fma(input[1],input[1],mul(input[0],input[0])));
 if(!number(sq))return ArenaImpulseDelta::Invalid;
 if(sq>64.f){
  const auto inverse=detail::inverse_root_estimate(sq),root=mul(inverse,sq);
  const auto refined=std::fma(std::fma(-inverse,root,std::bit_cast<float>(0x3f800001u)),mul(inverse,.5f),inverse);
  for(unsigned i=0;i<3;i++)impulse[i]=mul(mul(input[i],refined),8.f);
 }
 const auto estimate=detail::reciprocal_estimate(mass),base=mul(1.f,estimate);
 const auto inverse=std::fma(std::fma(-base,mass,1.f),estimate,base);
 const auto factor=mul(inverse,mul(float(s[0x7d]),std::bit_cast<float>(0x3c23d70au)));
 auto next=s;
 for(unsigned i=0;i<3;i++){
  const auto v=std::fma(impulse[i],factor,scalar(s,0x40+4*i));
  if(!number(v))return ArenaImpulseDelta::Invalid;
  scalar(next,0x40+4*i,v);
 }
 s=next;return ArenaImpulseDelta::Applied;
}
}
