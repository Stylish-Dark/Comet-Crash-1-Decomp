#include "comet/arena_ground_pair.hpp"
#include "arena_spu_math.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
using Vector=std::array<float,4>;
float value(unsigned bits){return std::bit_cast<float>(bits);}
float mul(float a,float b){volatile float r=a*b;return r;}
float add(float a,float b){volatile float r=a+b;return r;}
float sub(float a,float b){volatile float r=a-b;return r;}
struct Rounding {
 int previous=std::fegetround();
 Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("pair rounding unavailable");}
 ~Rounding(){std::fesetround(previous);}
};
float squared(const Vector &v){return std::fma(v[2],v[2],std::fma(v[1],v[1],mul(v[0],v[0])));}
float length(float s){
 const float estimate=detail::inverse_root_estimate(s),root=mul(estimate,s);
 return std::fma(std::fma(-estimate,root,value(0x3f800001)),mul(root,.5f),root);
}
float divide(float numerator,float denominator){
 const float estimate=detail::reciprocal_estimate(denominator),base=mul(numerator,estimate);
 return std::fma(std::fma(-base,denominator,numerator),estimate,base);
}
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
bool finite(const Vector&v){for(float f:v)if(!std::isfinite(f))return false;return true;}
}
Vector arena_ground_random_direction(ArenaRandom &random){
 Rounding rounding;
 auto angle=[&]{volatile float draw=static_cast<float>(random.next());return std::fma(divide(draw,value(0x4e7fffff)),value(0x40c90fda),0.f);};
 const auto a=trig(angle()),b=trig(angle());
 const Vector first{mul(a.cosine,1.f),mul(a.cosine,0.f),mul(a.cosine,0.f),mul(a.cosine,0.f)};
 const Vector second{std::fma(a.sine,0.f,first[0]),std::fma(a.sine,0.f,first[1]),std::fma(a.sine,1.f,first[2]),std::fma(a.sine,0.f,first[3])};
 Vector result;for(unsigned i=0;i<4;i++)result[i]=std::fma(b.sine,i==1?1.f:0.f,mul(second[i],b.cosine));
 return result;
}
std::optional<ArenaGroundPairResult> prepare_arena_ground_pair(const ArenaGroundPairQuery&q){
 if(!finite(q.position)||!finite(q.peer_position)||!finite(q.jitter)||!std::isfinite(q.radius)||q.radius<0||
    !std::isfinite(q.own_mass)||!std::isfinite(q.peer_mass)||q.own_mass<0||q.peer_mass<0||q.index==q.peer_index)return {};
 Rounding rounding;
 if(std::abs(squared(q.jitter)-1.f)>.01f||q.jitter[3]!=0)return {};
 ArenaGroundPairResult result;result.position=q.position;Vector delta;
 for(unsigned i=0;i<4;i++)delta[i]=sub(q.peer_position[i],result.position[i]);
 float s=squared(delta);if(!std::isfinite(s)||(!std::isnormal(s)&&s!=0))return {};
 float distance=s==0?0.f:length(s);
 if(distance<value(0x38d1b717)){
   result.position[2]=add(result.position[2],q.peer_index>q.index?value(0xbc23d70a):value(0x3c23d70a));
   for(unsigned i=0;i<4;i++)delta[i]=sub(q.peer_position[i],result.position[i]);
   s=squared(delta);if(!std::isnormal(s))return {};distance=length(s);
 }
 if(!std::isnormal(distance))return {};
 const float inverse=detail::reciprocal_estimate(distance);
 const float residual=std::fma(-distance,inverse,value(0x3f800001));
 Vector jittered;
 for(unsigned i=0;i<4;i++){
   const float base=mul(delta[i],inverse),direction=std::fma(residual,base,base);
   jittered[i]=std::fma(q.jitter[i],value(0x3dcccccc),direction);
 }
 s=squared(jittered);if(!std::isnormal(s))return {};
 const float estimate=detail::inverse_root_estimate(s),root=mul(estimate,s);
 const float normalize=std::fma(std::fma(-estimate,root,value(0x3f800001)),mul(estimate,.5f),estimate);
 for(unsigned i=0;i<4;i++)result.normal[i]=mul(jittered[i],normalize);
 // Radius subtraction is an FMA followed by a separate subtraction.
 result.gap=sub(std::fma(-float(q.peer_radius),value(0x3c23d70a),distance),q.radius);
 const float total=add(q.own_mass,q.peer_mass);if(!std::isfinite(total))return {};
 result.response_share=total>value(0x38d1b717)?divide(q.peer_mass,total):0.f;
 result.overlapping=value(0xb727c5ac)>result.gap;
 if(!finite(result.normal)||!finite(result.position)||!std::isfinite(result.gap)||!std::isfinite(result.response_share))return {};
 return result;
}
}
