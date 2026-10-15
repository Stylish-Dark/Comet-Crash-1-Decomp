#include "comet/arena_ground_base.hpp"
#include "arena_spu_math.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
float multiply(float a,float b){volatile float v=a*b;return v;}
struct Rounding {
 int previous=std::fegetround();
 Rounding(){if(previous<0 || std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("base arrival rounding unavailable");}
 ~Rounding(){std::fesetround(previous);}
};
}
std::optional<std::uint8_t> select_arena_ground_base(
    const std::array<float,4> &position,std::uint8_t team,float health,
    std::span<const ArenaGroundBase> bases){
 if(team>3 || bases.size()>4 || !std::isfinite(health) || health<=0)return {};
 for(unsigned i=0;i<3;++i)if(!std::isfinite(position[i]) || std::abs(position[i])>64)return {};
 for(const auto &b:bases)if(b.team>3 || b.cell.x>=24 || b.cell.z>=24)return {};
 Rounding rounding;
 std::optional<std::uint8_t> result;
 for(unsigned i=0;i<bases.size();++i){
  const auto &b=bases[i];if(!b.available || b.team==team)continue;
  volatile float x=(float(b.cell.x)+.5f)-position[0];
  volatile float y=0.f-position[1];
  volatile float z=(float(b.cell.z)+.5f)-position[2];
  const float squared=std::fma(float(z),float(z),std::fma(float(y),float(y),multiply(x,x)));
  // Zero/subnormal distances are a native near-center guard. The original
  // estimate sequence is compared only for finite-normal squared distances.
  float distance=0;
  if(std::isnormal(squared)){
   const float inverse=detail::inverse_root_estimate(squared);
   const float root=multiply(inverse,squared);
   const float residual=std::fma(-inverse,root,std::bit_cast<float>(0x3f800001u));
   distance=std::fma(residual,multiply(root,.5f),root);
  }
  if(distance<.5f)result=static_cast<std::uint8_t>(i);
 }
 return result;
}
}
