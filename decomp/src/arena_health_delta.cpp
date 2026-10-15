#include "comet/arena_health_delta.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace comet::decomp {
namespace {
unsigned word(const ArenaEntitySlot&s,unsigned o){unsigned v=0;for(unsigned i=0;i<4;++i)v=(v<<8)|s[o+i];return v;}
float scalar(const ArenaEntitySlot&s,unsigned o){return std::bit_cast<float>(word(s,o));}
void fp(ArenaEntitySlot&s,unsigned o,float f){const unsigned v=std::bit_cast<unsigned>(f);for(unsigned i=0;i<4;++i)s[o+i]=v>>(24-8*i);}
bool number(float f){return std::isfinite(f) && std::fpclassify(f)!=FP_SUBNORMAL;}
struct Rounding {
 int previous=std::fegetround();
 Rounding(){if(previous<0 || std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("health rounding unavailable");}
 ~Rounding(){std::fesetround(previous);}
};
float add(float a,float b){volatile float f=a+b;return f;}
}
bool accumulate_arena_health_delta(float &accumulator,float delta){
 if(!number(accumulator) || !number(delta))return false;
 const double exact=static_cast<double>(accumulator)+static_cast<double>(delta);
 if(std::abs(exact)>std::numeric_limits<float>::max())return false;
 Rounding rounding;const float next=add(accumulator,delta);
 if(!number(next))return false;
 accumulator=next;return true;
}
ArenaHealthDelta apply_arena_health_delta(ArenaEntitySlot &slot,float delta){
 const float health=scalar(slot,0x38),maximum=scalar(slot,0x64);
 if(!number(health) || !number(maximum) || !number(delta))return ArenaHealthDelta::Invalid;
 if(word(slot,0x60)&0x00100000u)return ArenaHealthDelta::Suppressed;
 Rounding rounding;const float sum=add(health,delta);
 const float next=maximum>sum?sum:maximum;
 if(!number(next))return ArenaHealthDelta::Invalid;
 fp(slot,0x38,next);return ArenaHealthDelta::Applied;
}
}
