#include "comet/arena_craft_flight.hpp"
#include <cfenv>
#include <bit>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
float normal(float v){return std::fpclassify(v)==FP_SUBNORMAL?0.f:v;}
float mul(float a,float b){volatile float v=normal(a)*normal(b);return normal(v);}
float sub(float a,float b){volatile float v=normal(a)-normal(b);return normal(v);}
float fused(float a,float b,float c){return normal(std::fma(normal(a),normal(b),normal(c)));}
struct Rounding {
 int previous=std::fegetround();
 Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("craft rounding unavailable");}
 ~Rounding(){std::fesetround(previous);}
};
}
void advance_arena_craft_flight(ArenaCraftFlightState& state,float x,float z,float elapsed,bool mode_flag,bool reduced_speed){
 if(!std::isfinite(x)||!std::isfinite(z)||std::abs(x)>1||std::abs(z)>1||
    !std::isfinite(elapsed)||elapsed<0||elapsed>.25f)throw std::invalid_argument("invalid craft flight input");
 for(const auto& v:{state.position,state.velocity})for(float f:v)
  if(!std::isfinite(f)||std::abs(f)>1000000)throw std::invalid_argument("unsupported craft flight state");
 Rounding rounding;auto next=state;
 // 104E0 selects the target velocity; 10530 advances with OLD velocity.
 std::array<float,4> desired{mul(x,mode_flag?10.f:40.f),0,mul(z,mode_flag?10.f:40.f),0};
 const float response=mode_flag?mul(elapsed,std::bit_cast<float>(0x40933333u)):1.f;
 for(unsigned i=0;i<4;++i){
  next.position[i]=fused(state.velocity[i],elapsed,state.position[i]);
  if(mode_flag&&reduced_speed)desired[i]=mul(desired[i],std::bit_cast<float>(0x3f333333u));
  next.velocity[i]=fused(sub(desired[i],state.velocity[i]),response,state.velocity[i]);
 }
 // 105F0 clears the Y lane after integrating the old velocity.
 next.velocity[1]=0;
 for(const auto& v:{next.position,next.velocity})for(float f:v)
  if(!std::isfinite(f)||std::abs(f)>1000000)throw std::invalid_argument("craft flight overflow");
 state=next;
}
}
