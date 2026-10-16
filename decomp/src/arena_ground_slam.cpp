#include "comet/arena_ground_slam.hpp"
#include "arena_spu_math.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
namespace comet::decomp {
namespace {
float bits(unsigned v){return std::bit_cast<float>(v);}
float multiply(float a,float b){volatile float v=a*b;return v;}
struct Rounding{int previous;~Rounding(){std::fesetround(previous);}};
}
std::optional<bool> arena_ground_slam_ready(const std::array<float,4>&impulse){
 for(float v:impulse)if(!std::isfinite(v))return {};
 if(impulse[3]!=0)return {};
 double bound=0;for(unsigned i=0;i<3;i++)bound+=double(impulse[i])*impulse[i];
 if(bound>std::numeric_limits<float>::max())return {};
 const int previous=std::fegetround();if(previous<0||std::fesetround(FE_TOWARDZERO)!=0)return {};
 Rounding rounding{previous};
 const float squared=std::fma(impulse[2],impulse[2],std::fma(impulse[1],impulse[1],multiply(impulse[0],impulse[0])));
 if(!std::isfinite(squared))return {};
 if(squared<=1.f)return false;
 const float estimate=detail::inverse_root_estimate(squared),base=multiply(estimate,squared);
 const float length=std::fma(std::fma(-estimate,base,bits(0x3f800001)),multiply(base,.5f),base);
 if(!(length>1.f))return false;
 const float reciprocal=detail::reciprocal_estimate(length),normal_y=multiply(impulse[1],reciprocal);
 const float refined_y=std::fma(std::fma(-length,reciprocal,bits(0x3f800001)),normal_y,normal_y);
 if(!std::isfinite(refined_y))return {};
 return refined_y>bits(0x3f666666);
}
}
