#include "comet/arena_ground_height.hpp"
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
 Rounding(){if(previous<0 || std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("ground height rounding unavailable");}
 ~Rounding(){std::fesetround(previous);}
};
}
std::optional<std::array<float,4>> steer_arena_ground_height(const ArenaGroundHeightQuery &q){
 for(const auto *v:{&q.position,&q.velocity,&q.direction})for(float f:*v)if(!std::isfinite(f))return {};
 if(!q.enabled)return q.direction;
 Rounding rounding;
 // 143A0..1441C: unit-height spring with vertical velocity damping, then
 // normalize the complete XYZ steering vector using the original estimate.
 volatile float difference=1.f-q.position[1];
 const float correction=std::fma(q.velocity[1],-1.f,float(difference));
 std::array<float,4> direction{};
 for(unsigned i=0;i<4;++i)direction[i]=std::fma(i==1?correction:0.f,4.f,q.direction[i]);
 const float squared=std::fma(direction[2],direction[2],std::fma(direction[1],direction[1],multiply(direction[0],direction[0])));
 // Exactly zero and non-normal SPU estimate inputs have no supported native
 // IEEE equivalent. Reject them before mutating live entity state.
 if(!std::isnormal(squared)||squared<=0)return {};
 const float inverse=detail::inverse_root_estimate(squared);
 const float root=multiply(inverse,squared);
 const float residual=std::fma(-inverse,root,std::bit_cast<float>(0x3f800001u));
 const float refined=std::fma(residual,multiply(inverse,.5f),inverse);
 for(float &f:direction){f=multiply(f,refined);if(!std::isfinite(f))return {};}
 return direction;
}
}
