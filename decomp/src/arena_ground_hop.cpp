#include "comet/arena_ground_hop.hpp"
#include "arena_spu_math.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
namespace comet::decomp {
namespace {
using V=std::array<float,4>;
float value(unsigned x){return std::bit_cast<float>(x);}
float mul(float a,float b){volatile float r=a*b;return r;}
float add(float a,float b){volatile float r=a+b;return r;}
float sub(float a,float b){volatile float r=a-b;return r;}
float square(const V&v){return std::fma(v[2],v[2],std::fma(v[1],v[1],mul(v[0],v[0])));}
float inverse(float squared){const float e=detail::inverse_root_estimate(squared);return std::fma(std::fma(-e,mul(e,squared),value(0x3f800001)),mul(e,.5f),e);}
struct Trig{float sine,cosine;};
// Original range reduction and two short polynomials, not host libm trig.
Trig trig(float angle){
 const float quadrant_float=add(mul(angle,value(0x3f22f983)),std::copysign(.5f,angle));
 const int quadrant=static_cast<int>(quadrant_float);
 const float q=static_cast<float>(quadrant);
 const float reduced=std::fma(-q,value(0x33a22168),std::fma(-q,value(0x3fc90fd9),angle));
 const float s=mul(reduced,reduced);
 const float cosine=std::fma(std::fma(std::fma(value(0xbab24992),s,value(0x3d2aa035)),s,value(0xbeffffdf)),s,1.f);
 const float sine=std::fma(std::fma(std::fma(value(0xb94c8c6d),s,value(0x3c088342)),s,value(0xbe2aaaa1)),mul(s,reduced),reduced);
 auto select=[&](int k){float r=(k&1)?cosine:sine;return (k&2)?-r:r;};
 return {select(quadrant),select(quadrant+1)};
}

V circle(ArenaRandom &random){
 volatile float draw=static_cast<float>(random.next());const float estimate=detail::reciprocal_estimate(value(0x4e7fffff));const float base=mul(draw,estimate);
 const float ratio=std::fma(std::fma(-base,value(0x4e7fffff),draw),estimate,base);
 const auto t=trig(std::fma(ratio,value(0x40c90fda),0.f));V result{};
 for(unsigned i=0;i<4;i++)result[i]=std::fma(i==0?1.f:0.f,t.sine,mul(i==2?1.f:0.f,t.cosine));
 return result;
}
struct Rounding{int previous;~Rounding(){std::fesetround(previous);}};
}
std::optional<ArenaGroundHopResult> advance_arena_ground_hop(const ArenaGroundHopQuery&q,ArenaRandom&random){
 if(!std::isfinite(q.elapsed)||q.elapsed<=0||q.elapsed>.25f||!std::isfinite(q.timer)||q.timer<-.25f||q.timer>4||!std::isnormal(q.speed_limit)||q.speed_limit<=0)return {};
 for(const auto*v:{&q.velocity,&q.steering,&q.lookahead_steering,&q.external_acceleration})for(float f:*v)if(!std::isfinite(f)||std::abs(f)>1e10f)return {};
 if(q.velocity[3]!=0||q.steering[3]!=0||q.lookahead_steering[3]!=0||q.external_acceleration[3]!=0)return {};
 const int previous=std::fegetround();if(previous<0||std::fesetround(FE_TOWARDZERO)!=0)return {};Rounding rounding{previous};
 auto next_random=random;ArenaGroundHopResult result;result.velocity=q.velocity;result.timer=sub(q.timer,q.elapsed);result.hopped=result.timer<0;
 for(unsigned i=0;i<4;i++)result.acceleration[i]=mul(q.external_acceleration[i],value(0x3f4ccccd));
 if(result.hopped){
  result.timer=add(result.timer,4.f);if(q.flags&2)result.position_y=value(0xbdcccccc);
  V direction{};for(unsigned i=0;i<4;i++)direction[i]=std::fma(q.lookahead_steering[i],.5f,q.steering[i]);
  const float squared=square(direction);if(!std::isnormal(squared))return {};const float scale=inverse(squared);
  for(auto &v:direction)v=mul(v,scale);
  const V jitter=circle(next_random);const float spread=(q.flags&2)?value(0x3d23d70a):value(0x3dcccccc);
  for(unsigned i=0;i<4;i++){
   const float directed=std::fma(jitter[i],spread,direction[i]);
   result.velocity[i]=add(result.velocity[i],std::fma(directed,value(0x3fb33333),i==1?value(0x3f8ccccc):0.f));
  }
 }else if(q.grounded){for(unsigned i=0;i<4;i++)result.acceleration[i]=std::fma(result.velocity[i],-5.f,result.acceleration[i]);}
 if(!q.grounded){
  if(!result.hopped && result.velocity[1]<0)result.acceleration[1]=add(result.acceleration[1],-6.f);
  const float squared=square(result.velocity);const float scale=squared==0?0:inverse(squared);V normal{};
  for(unsigned i=0;i<4;i++)normal[i]=mul(result.velocity[i],scale);
  const float alignment=std::fma(q.steering[2],normal[2],std::fma(normal[1],q.steering[1],mul(q.steering[0],normal[0])));
  if(squared!=0 && value(0x3dcccccc)>alignment)for(unsigned i=0;i<4;i++)result.acceleration[i]=std::fma(q.steering[i],.5f,result.acceleration[i]);
 }
 for(unsigned i=0;i<4;i++)result.velocity[i]=std::fma(result.acceleration[i],q.elapsed,result.velocity[i]);
 const auto&v=result.velocity;const float squared=std::fma(v[2],v[2],std::fma(mul(v[1],0.f),mul(v[1],0.f),mul(v[0],v[0])));
 result.horizontal_speed_squared_before_limit=squared;result.speed_limited=squared>mul(q.speed_limit,q.speed_limit);
 if(result.speed_limited){const float scale=inverse(squared);for(unsigned i=0;i<4;i++)result.velocity[i]=std::fma(mul(mul(v[i],(i==0||i==2)?1.f:0.f),scale),q.speed_limit,mul(v[i],i==1?1.f:0.f));}
 for(const auto*vec:{&result.velocity,&result.acceleration})for(float f:*vec)if(!std::isfinite(f))return {};
 random=next_random;return result;
}
}
