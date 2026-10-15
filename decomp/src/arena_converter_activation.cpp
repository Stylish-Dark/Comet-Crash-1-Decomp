#include "comet/arena_converter_activation.hpp"
#include "arena_spu_math.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
namespace comet::decomp {
namespace {
float bits(unsigned v){return std::bit_cast<float>(v);}
float multiply(float a,float b){volatile float v=a*b;return v;}
float add(float a,float b){volatile float v=a+b;return v;}
float subtract(float a,float b){volatile float v=a-b;return v;}
float scalar(const ArenaEntitySlot&s,unsigned o){unsigned v=0;for(unsigned i=0;i<4;i++)v=(v<<8)|s[o+i];return bits(v);}
void put(ArenaEntitySlot&s,unsigned o,float f){unsigned v=std::bit_cast<unsigned>(f);for(unsigned i=0;i<4;i++)s[o+i]=v>>(24-8*i);}
float sine(float phase){
 const float scaled=multiply(phase,bits(0x3f22f983));
 const int quadrant=static_cast<int>(add(scaled,std::copysign(.5f,scaled)));
 const float q=static_cast<float>(quadrant);
 const float reduced=std::fma(-q,bits(0x33a22168),std::fma(-q,bits(0x3fc90fd9),phase));
 const float square=multiply(reduced,reduced);
 const float cosine=std::fma(std::fma(std::fma(square,bits(0xbab24992),bits(0x3d2aa035)),square,bits(0xbeffffdf)),square,1.f);
 const float sine=std::fma(std::fma(std::fma(square,bits(0xb94c8c6d),bits(0x3c088342)),square,bits(0xbe2aaaa1)),multiply(square,reduced),reduced);
 const float value=(quadrant&1)?cosine:sine;return(quadrant&2)?-value:value;
}
struct Rounding {int previous;~Rounding(){std::fesetround(previous);}};
}
std::optional<float> advance_arena_converter_activation(ArenaEntitySlot &slot,float elapsed){
 float phase=scalar(slot,0x98);
 const float cooldown=scalar(slot,0x8c);
 if(slot[15]!=45 || ((slot[0x33]>>2)&7)!=5 || !std::isfinite(elapsed) || elapsed<0 || elapsed>3 ||
    !std::isfinite(cooldown) || (cooldown>0 && !std::isfinite(scalar(slot,0x18))) ||
    !std::isfinite(phase) || phase<0 || phase>bits(0x40c90fda))return {};
 const int previous=std::fegetround();if(previous<0 || std::fesetround(FE_TOWARDZERO)!=0)return {};
 Rounding rounding{previous};
 if(cooldown>0){
   const float core18=multiply(scalar(slot,0x18),bits(0x3f199999));
   if(!std::isfinite(core18))return {};
   put(slot,0x8c,subtract(cooldown,elapsed));slot[0xbb]=0;return core18;
 }
 const float numerator=multiply(multiply(elapsed,4.f),bits(0x3fc90fda));
 const float estimate=detail::reciprocal_estimate(3.f),base=multiply(numerator,estimate);
 const float increment=std::fma(std::fma(-base,3.f,numerator),estimate,base);
 phase=add(phase,increment);
 // The original subtracts once, strictly above 2*pi; it does not use fmod.
 if(phase>bits(0x40c90fda))phase=subtract(phase,bits(0x40c90fda));
 const float wave=sine(phase),scaled=multiply(wave,9.f);
 put(slot,0x98,phase);slot[0xbb]=wave>bits(0x3dcccccc);
 return bits(0x3f333333)>scaled?bits(0x3f333333):scaled;
}
}
