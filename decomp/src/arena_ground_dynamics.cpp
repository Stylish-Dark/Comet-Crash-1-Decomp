#include "comet/arena_ground_dynamics.hpp"
#include "arena_spu_math.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
float multiply(float a,float b) { volatile float v=a*b;return v; }
float add(float a,float b) { volatile float v=a+b;return v; }
struct Rounding {
  int previous=std::fegetround();
  Rounding() {
    if(previous<0 || std::fesetround(FE_TOWARDZERO)!=0)
      throw std::runtime_error("ground dynamics rounding unavailable");
  }
  ~Rounding() { std::fesetround(previous); }
};
}
std::optional<ArenaGroundDynamicsResult>
advance_arena_ground_velocity(const ArenaGroundDynamicsQuery &q) {
  if(!std::isfinite(q.elapsed)||q.elapsed<=0||q.elapsed>.25f ||
     !std::isfinite(q.steering_acceleration)||q.steering_acceleration<0 ||
     !std::isnormal(q.speed_limit)||q.speed_limit<=0 ||
     !(q.kind==40||q.kind==41||(q.kind>=43&&q.kind<=47))) return {};
  for(const auto *v:{&q.velocity,&q.steering,&q.external_acceleration})
    for(float f:*v) if(!std::isfinite(f)) return {};
  Rounding rounding;
  ArenaGroundDynamicsResult result;
  // 159B4/159C0: acceleration = fma(direction, table[0], velocity * -0.01).
  // 159BC..159D8: context bit 0x20 adds the external vector except kind 47.
  const float drag=std::bit_cast<float>(0xbc23d70au);
  for(unsigned i=0;i<4;++i) {
    result.acceleration[i]=std::fma(q.steering[i],q.steering_acceleration,
                                  multiply(q.velocity[i],drag));
    if((q.context_flags&32u)!=0 && q.kind!=47)
      result.acceleration[i]=add(result.acceleration[i],q.external_acceleration[i]);
    result.velocity[i]=std::fma(result.acceleration[i],q.elapsed,q.velocity[i]);
    if(!std::isfinite(result.acceleration[i])||!std::isfinite(result.velocity[i]))return {};
  }
  const auto &v=result.velocity;
  // Dot products include the masked zero Y lane in the original FMA chain.
  const float squared=std::fma(v[2],v[2],std::fma(0.f,0.f,multiply(v[0],v[0])));
  const float limit_squared=multiply(q.speed_limit,q.speed_limit);
  if(!std::isfinite(squared)||!std::isfinite(limit_squared))return {};
  result.horizontal_speed_squared_before_limit=squared;
  result.speed_limited=squared>limit_squared;
  if(result.speed_limited) {
    if(!std::isnormal(squared))return {};
    const float inverse=detail::inverse_root_estimate(squared);
    const float root=multiply(inverse,squared);
    const float half_inverse=multiply(inverse,.5f);
    const float residual=std::fma(-inverse,root,std::bit_cast<float>(0x3f800001u));
    const float refined=std::fma(residual,half_inverse,inverse);
    // 162EC/162F0: normalize horizontal first, then multiply by cap in an FMA.
    // Multiplication masks preserve the signs of zero lanes (including W).
    for(unsigned i=0;i<4;++i) {
      const float horizontal=multiply(v[i],(i==0||i==2)?1.f:0.f);
      const float perpendicular=multiply(v[i],i==1?1.f:0.f);
      result.velocity[i]=std::fma(multiply(horizontal,refined),q.speed_limit,perpendicular);
    }
  }
  return result;
}
} // namespace comet::decomp
