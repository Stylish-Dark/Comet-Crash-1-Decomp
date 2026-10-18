#include "comet/arena_weapon_aim.hpp"
#include "arena_spu_math.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp { namespace {
using V=std::array<float,4>;
float c(std::uint32_t x){return std::bit_cast<float>(x);}
float mul(float a,float b){volatile float r=a*b;return r;}
float add(float a,float b){volatile float r=a+b;return r;}
float sub(float a,float b){volatile float r=a-b;return r;}
float ma(float a,float b,float d){return std::fma(a,b,d);}
float nm(float a,float b,float d){return std::fma(-a,b,d);}
float inv(float s){float e=detail::inverse_root_estimate(s);return ma(nm(e,mul(e,s),c(0x3f800001)),mul(e,.5f),e);}
float root(float s){if(s==0)return 0;float e=detail::inverse_root_estimate(s),v=mul(e,s);return ma(nm(e,v,c(0x3f800001)),mul(v,.5f),v);}
float dot3(const V&a,const V&b){return ma(a[2],b[2],ma(a[1],b[1],mul(a[0],b[0])));}
V norm3(V v){float f=inv(dot3(v,v));for(auto &x:v)x=mul(x,f);return v;}
V product(const V&a,const V&b){
 V v{};for(int i=0;i<3;i++){int j=(i+1)%3,k=(i+2)%3;float r=ma(b[3],a[i],mul(a[3],b[i]));r=ma(a[j],b[k],r);v[i]=nm(a[k],b[j],r);}
 v[3]=sub(nm(a[2],b[2],mul(a[3],b[3])),ma(a[0],b[0],mul(a[1],b[1])));
 return v;
}
float sine(float x,bool cosine=false){
 float scaled=mul(x,c(0x3f22f983));float nearest=add(scaled,std::copysign(.5f,scaled));int n=int(nearest);float reduced=nm(float(n),c(0x3fc90fd9),x);reduced=nm(float(n),c(0x33a22168),reduced);float s=mul(reduced,reduced);
 float cos=ma(ma(ma(s,c(0xbab24992),c(0x3d2aa035)),s,c(0xbeffffdf)),s,1);
 float sin=ma(ma(ma(s,c(0xb94c8c6d),c(0x3c088342)),s,c(0xbe2aaaa1)),mul(s,reduced),reduced);
 int quadrant=(n+(cosine?1:0))&3;float r=(quadrant&1)?cos:sin;return (quadrant&2)?-r:r;
}
// The zero-root and unit clamp are finite analytic limits of the SPU math.
float acos_original(float input){
 float d=std::min(1.f,std::fabs(input));
 float p=ma(d,c(0xbaa57a2c),c(0x3bda90c5));p=ma(p,d,c(0xbc8bfc66));p=ma(p,d,c(0x3cfd10f8));
 float q=ma(d,c(0xbd4d8392),c(0x3db63a9e));q=ma(q,d,c(0xbe5bbfca));q=ma(q,d,c(0x3fc90fda));float s=mul(d,d);
 float v=root(sub(1,d)),poly=ma(p,mul(s,s),q);return input<0?nm(v,poly,c(0x40490fda)):mul(v,poly);
}
struct Rounding{int old=std::fegetround();Rounding(){if(old==-1||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("weapon aim rounding unavailable");}~Rounding(){std::fesetround(old);}};
// Preserve the original add/subtract rounding, even for angles already in range.
float output_wrap(float x){const float pi=c(0x40490fda),tau=c(0x40c90fda);if(pi>x)x=add(x,tau);if(x>pi)x=sub(x,tau);return x;}
float wrap(float x){const float pi=c(0x40490fda),tau=c(0x40c90fda);if(x < -pi)x=add(x,tau);if(x>pi)x=sub(x,tau);return x;}
}
std::optional<ArenaWeaponAimResult> prepare_arena_weapon_aim(const ArenaWeaponAimQuery&q){
 for(float x:q.direction)if(!std::isfinite(x))return std::nullopt;
 for(float x:{q.yaw,q.pitch,q.yaw_step,q.pitch_step,q.pitch_min,q.pitch_max})if(!std::isfinite(x))return std::nullopt;
 const float pi=c(0x40490fda);
 if(q.direction[3]!=0||std::fabs(q.yaw)>pi||std::fabs(q.pitch)>pi||q.yaw_step<0||q.pitch_step<0||q.yaw_step>pi||q.pitch_step>pi||q.pitch_min < -pi||q.pitch_max>pi||q.pitch_min>q.pitch_max)return std::nullopt;
 Rounding rounding;float length=dot3(q.direction,q.direction);
 if(length!=0&&std::fabs(sub(length,1))>.001f)return std::nullopt;
 ArenaWeaponAimResult r{};r.yaw=q.yaw;r.pitch=q.pitch;
 V horizontal{q.direction[0],0,q.direction[2],0};float sq=dot3(horizontal,horizontal);
 if(sq>c(0x38d1b717)){
   horizontal=norm3(horizontal);
   float desired_yaw=acos_original(-horizontal[2]);if(horizontal[0]>=0)desired_yaw=-desired_yaw;
   float desired_pitch=acos_original(dot3(horizontal,q.direction));if(q.direction[1]<0)desired_pitch=-desired_pitch;
   desired_pitch=std::clamp(desired_pitch,q.pitch_min,q.pitch_max);
   r.yaw=output_wrap(add(r.yaw,std::clamp(wrap(sub(desired_yaw,r.yaw)),-q.yaw_step,q.yaw_step)));
   r.pitch=output_wrap(add(r.pitch,std::clamp(sub(desired_pitch,r.pitch),-q.pitch_step,q.pitch_step)));
 }
 float yh=mul(r.yaw,.5f),ph=mul(r.pitch,.5f);
 r.source_orientation={0,sine(yh),0,sine(yh,true)};
 V pitch{sine(ph),0,0,sine(ph,true)};
 r.muzzle_orientation=product(r.source_orientation,pitch);
 const V &v=r.muzzle_orientation;
 // Preserve the 1A428..1A450 operation order; reassociation changes the fire gate.
 V rotated{-v[1],v[0],-v[3],0},forward{};
 for(int i=0;i<3;i++){int j=(i+1)%3,k=(i+2)%3;float t=ma(v[3],rotated[i],mul(-v[2],v[i]));t=ma(v[j],rotated[k],t);forward[i]=nm(v[k],rotated[j],t);}
 r.remaining_angle=acos_original(dot3(forward,q.direction));
 for(float x:r.muzzle_orientation)if(!std::isfinite(x))return std::nullopt;
 return r;
}
}
