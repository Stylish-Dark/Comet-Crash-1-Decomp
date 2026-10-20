#include "comet/arena_craft_facing.hpp"
#include "comet/arena_weapon_aim.hpp"
#include "arena_spu_math.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp { namespace {
float bits(unsigned b){return std::bit_cast<float>(b);}
float mul(float a,float b){volatile float result=a*b;return result;}
float fused(float a,float b,float c){return std::fma(a,b,c);}
struct Rounding {
 int old=std::fegetround();
 Rounding(){if(old<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("craft facing rounding unavailable");}
 ~Rounding(){std::fesetround(old);}
};
}
std::array<float,4> advance_arena_craft_facing(ArenaCraftFacingState& state,
 float x,float z,float elapsed,bool mode_flag,bool reduced_speed){
 if(!std::isfinite(x)||!std::isfinite(z)||std::abs(x)>1||std::abs(z)>1||
    !std::isfinite(elapsed)||elapsed<0||elapsed>.25f||!std::isfinite(state.yaw)||
    std::abs(state.yaw)>bits(0x40490fda))throw std::invalid_argument("invalid craft facing input");
 float squared=0;for(float q:state.pending_orientation){if(!std::isfinite(q))throw std::invalid_argument("invalid craft facing quaternion");squared+=q*q;}
 if(!std::isfinite(squared)||std::abs(squared-1)>.001f)throw std::invalid_argument("invalid craft facing quaternion");
 const auto visible=state.pending_orientation;
 Rounding rounding;
 std::array<float,4> desired{mul(x,mode_flag?10.f:40.f),0,mul(z,mode_flag?10.f:40.f),0};
 if(mode_flag&&reduced_speed)for(auto& f:desired)f=mul(f,bits(0x3f333333));
 const float s=fused(desired[2],desired[2],fused(desired[1],desired[1],mul(desired[0],desired[0])));
 if(s==0)return visible;
 // 11558..115A4 length and strict .01 gate; 115DC..11610 reciprocal refinement.
 const float inverse=detail::inverse_root_estimate(s),root=mul(inverse,s);
 const float error=fused(-inverse,root,bits(0x3f800001));
 const float length=fused(error,mul(root,.5f),root);
 if(!(length>bits(0x3c23d70a)))return visible;
 const float estimate=detail::reciprocal_estimate(length);
 const float residual=fused(-length,estimate,bits(0x3f800001));
 ArenaWeaponAimQuery aim;
 for(unsigned i=0;i<4;++i){const float product=mul(desired[i],estimate);aim.direction[i]=fused(residual,product,product);}
 aim.yaw=state.yaw;aim.yaw_step=mul(elapsed,bits(0x41490fda));
 // 115B0..11614 supply zero pitch, step and pitch limits to original 1A118.
 const auto result=prepare_arena_weapon_aim(aim);
 if(!result)throw std::invalid_argument("unsupported craft facing arithmetic");
 state.yaw=result->yaw;state.pending_orientation=result->source_orientation;
 return visible;
}
}
