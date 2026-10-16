#include "comet/arena_area_effect.hpp"
#include "arena_spu_math.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
float bits(unsigned x){return std::bit_cast<float>(x);}
float mul(float a,float b){volatile float v=a*b;return v;}
float add(float a,float b){volatile float v=a+b;return v;}
float sub(float a,float b){volatile float v=a-b;return v;}
bool number(float f){return std::isfinite(f) && std::fpclassify(f)!=FP_SUBNORMAL;}
struct Rounding{int previous=std::fegetround();Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("area effect rounding unavailable");}~Rounding(){std::fesetround(previous);}};
void validate(const std::array<float,4>& center,ArenaGridPosition cell,float radius){
 for(unsigned i=0;i<3;++i)if(!number(center[i]) || std::abs(center[i])>64)throw std::invalid_argument("invalid area effect center");
 if(!number(radius)||radius<.0001f||radius>64||cell.x>=24||cell.z>=24)throw std::invalid_argument("invalid area effect geometry");
}
float reciprocal(float x){const float e=detail::reciprocal_estimate(x),q=mul(1,e);return std::fma(std::fma(-q,x,1.f),e,q);}
float length(const std::array<float,4>& v){
 const float sq=std::fma(v[2],v[2],std::fma(v[1],v[1],mul(v[0],v[0])));
 if(sq==0)return 0;
 const float e=detail::inverse_root_estimate(sq),r=mul(e,sq);
 return std::fma(std::fma(-e,r,bits(0x3f800001)),mul(r,.5f),r);
}
// Native scalar evaluation of the recovered logarithm/exponential approximation.
// Both callers use power 3; preserving the approximation retains its rounding.
float cubic_power(float x){
 if(x==0)return 0;
 const unsigned b=std::bit_cast<unsigned>(x);
 float m=bits((b&~0x7f800000u)|0x3f000000u);
 int n=int((b>>23)&255)-126;
 if(bits(0x3f3504f3)>m){m=add(m,m);--n;}
 const float u=sub(m,1.f),den=add(m,1.f),e=detail::reciprocal_estimate(den);
 const float z0=mul(u,e),z=std::fma(std::fma(-den,e,bits(0x3f800001)),z0,z0),zz=mul(z,z);
 float p=std::fma(zz,bits(0x3e990214),bits(0x3eccae10));
 p=std::fma(p,zz,bits(0x3f2aaabf));
 p=std::fma(p,zz,-u);
 const float correction=std::fma(u,bits(0x32a5705f),mul(mul(z,p),bits(0x3fb8aa3b)));
 const float logarithm=add(std::fma(u,bits(0x3fb8aa3a),float(n)),correction);
 const float exponent=mul(3.f,logarithm);
 if(exponent < -126.f)return 0;
 const int whole=int(add(exponent,exponent>=0?bits(0x3f7fffff):0.f));
 const float t=mul(sub(float(whole),exponent),bits(0x3f317217)),tt=mul(t,t);
 const float p1=std::fma(t,bits(0xb9142e40),bits(0x3aae4f6f));
 const float p2=std::fma(t,bits(0xbe2aaa4f),bits(0x3efffffd));
 const float p3=std::fma(t,p1,bits(0xbc08026d));
 const float p4=std::fma(t,p2,-1.f);
 const float p5=std::fma(t,p3,bits(0x3d2aa0e5));
 const float p6=std::fma(t,p4,1.f);
 const float fraction=std::fma(mul(tt,tt),p5,p6);
 return mul(fraction,bits(unsigned(whole+127)<<23));
}
}
bool arena_area_effect_cell_overlaps(const std::array<float,4>& center,ArenaGridPosition cell,float radius){
 validate(center,cell,radius);Rounding rounding;
 const std::array<float,4> v{sub(center[0],add(float(cell.x),.5f)),0,sub(center[2],add(float(cell.z),.5f)),0};
 return 0>sub(sub(length(v),radius),bits(0x3f350529));
}
std::optional<ArenaAreaEffectHit> evaluate_arena_area_effect_cubic(const ArenaAreaEffectQuery&q){
 validate(q.center,q.cell,q.radius);
 if(!number(q.damage)||!number(q.impulse_strength)||q.damage<0||q.impulse_strength<0)throw std::invalid_argument("invalid area effect magnitude");
 const auto &e=q.entry;const unsigned stage=(e[7]>>2)&7,owner=e[7]>>5;
 if(!(q.stage_mask&(1u<<stage)) || !(q.owner_mask&(1u<<owner)) || (q.exclude_marker_two&&e[3]==2))return std::nullopt;
 Rounding rounding;const float percent=bits(0x3c23d70a);
 const std::array<float,4> base{sub(float(q.cell.x),.5f),-.5f,sub(float(q.cell.z),.5f),0};
 std::array<float,4> direction{};for(unsigned axis=0;axis<3;++axis)direction[axis]=sub(std::fma(float(e[4+axis]),percent,base[axis]),q.center[axis]);
 const float distance=length(direction),peer_radius=mul(float(e[2]),percent);
 if(!(percent>sub(sub(distance,q.radius),peer_radius)))return std::nullopt;
 const float surface=sub(distance,peer_radius),ratio=mul(surface<0?0:surface,reciprocal(q.radius));
 const float falloff=sub(1.f,cubic_power(ratio));
 ArenaAreaEffectHit hit;hit.index=(unsigned(e[0])<<8)|e[1];hit.health_delta=-mul(falloff,q.damage);
 if(distance>bits(0x38d1b717)){
  const float magnitude=mul(mul(falloff,q.impulse_strength),10.f),inv=detail::reciprocal_estimate(distance);
  for(unsigned axis=0;axis<4;++axis){const float scaled=mul(direction[axis],magnitude),v=mul(scaled,inv);hit.impulse[axis]=std::fma(std::fma(-distance,inv,bits(0x3f800001)),v,v);}
 }
 if(!number(hit.health_delta))throw std::invalid_argument("area effect magnitude overflow");
 for(float f:hit.impulse)if(!number(f))throw std::invalid_argument("area effect impulse overflow");
 return hit;
}
}
