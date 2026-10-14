#include "comet/arena_ground_orientation.hpp"
#include "arena_spu_math.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
using V=std::array<float,4>;
float c(std::uint32_t x){return std::bit_cast<float>(x);}
float mul(float a,float b){volatile float r=a*b;return r;}
float add(float a,float b){volatile float r=a+b;return r;}
float sub(float a,float b){volatile float r=a-b;return r;}
float ma(float a,float b,float d){return std::fma(a,b,d);}
float nm(float a,float b,float d){return std::fma(-a,b,d);}
float inv(float s){float e=detail::inverse_root_estimate(s);return ma(nm(e,mul(e,s),c(0x3f800001)),mul(e,.5f),e);}
float root(float s){float e=detail::inverse_root_estimate(s),v=mul(e,s);return ma(nm(e,v,c(0x3f800001)),mul(v,.5f),v);}
float reciprocal(float s){float e=detail::reciprocal_estimate(s);return ma(nm(s,e,c(0x3f800001)),e,e);}
float dot3(const V&a,const V&b){return ma(a[2],b[2],ma(a[1],b[1],mul(a[0],b[0])));}
V cross(const V&a,const V&b){return {nm(a[2],b[1],mul(a[1],b[2])),nm(a[0],b[2],mul(a[2],b[0])),nm(a[1],b[0],mul(a[0],b[1])),0};}
V norm3(V v){float f=inv(dot3(v,v));for(auto &x:v)x=mul(x,f);return v;}
V norm4(V v){float a=ma(v[1],v[1],mul(v[0],v[0]));float b=ma(v[3],v[3],mul(v[2],v[2]));float f=inv(add(a,b));for(auto &x:v)x=mul(x,f);return v;}
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
float atan2_original(float numerator,float denominator,bool negative){
 float ratio=mul(numerator,reciprocal(denominator));bool large=std::fabs(ratio)>1;float offset=large?std::copysign(c(0x3fc90fda),ratio):0;
 float x=large?-reciprocal(ratio):ratio,s=mul(x,x),s2=mul(s,s),s4=mul(s2,s2);
 float p=ma(s,c(0x3b3bd74a),c(0xbc846e02));p=ma(p,s,c(0x3d2fc1fe));p=ma(p,s,c(0xbd9a3174));p=ma(p,s,c(0x3dda3d83));
 float q=ma(s,c(0xbe117fc7),c(0x3e4cbbe5));q=ma(q,s,c(0xbeaaaa6c));q=ma(q,mul(s,x),add(x,offset));
 float a=ma(p,mul(s4,x),q);if(denominator<0)a=add(numerator<0?-c(0x40490fda):c(0x40490fda),a);return negative?-std::fabs(a):std::fabs(a);
}
float bank_angle(const V&q){
 V conjugate{-q[0],-q[1],-q[2],q[3]};V forward=product(product(q,{0,0,1,0}),conjugate),up=product(product(q,{0,1,0,0}),conjugate);forward[3]=up[3]=0;
 V side=cross(forward,{0,1,0,0});
 if(dot3(side,side)==0)return 0;
 side=norm3(side);V reference=cross(side,forward),swing=cross(up,reference);
 if(dot3(swing,swing)==0)return 0;
 float denominator=dot3(up,reference),numerator=root(dot3(swing,swing));bool negative=dot3(swing,forward)<=0;return atan2_original(numerator,denominator,negative);
}
float acos_original(float d){
 float p=ma(d,c(0xbaa57a2c),c(0x3bda90c5));p=ma(p,d,c(0xbc8bfc66));p=ma(p,d,c(0x3cfd10f8));
 float q=ma(d,c(0xbd4d8392),c(0x3db63a9e));q=ma(q,d,c(0xbe5bbfca));q=ma(q,d,c(0x3fc90fda));float s=mul(d,d);
 return mul(root(sub(1,d)),ma(p,mul(s,s),q));
}
struct Rounding{int old=std::fegetround();Rounding(){if(old==-1||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("orientation floating point rounding unavailable");}~Rounding(){std::fesetround(old);}};
}
std::optional<V> prepare_arena_ground_orientation(const ArenaGroundOrientationQuery &q){
 for(float x:q.velocity)if(!std::isfinite(x))return std::nullopt;
 for(float x:q.quaternion)if(!std::isfinite(x))return std::nullopt;
 float prior_length=0;for(float x:q.quaternion)prior_length+=x*x;
 if(!std::isfinite(prior_length)||std::fabs(prior_length-1)>1e-3f)return std::nullopt;
 const bool ordinary=q.kind==40||q.kind==41||q.kind==43||q.kind==44||q.kind==45||q.kind==47;
 if(!ordinary||q.velocity[3]!=0||q.steering_direction[3]!=0||!std::isfinite(q.prelimit_speed_squared)||q.prelimit_speed_squared<0||!std::isfinite(q.elapsed)||q.elapsed<0||q.elapsed>.25f||!std::isfinite(q.turn_rate)||q.turn_rate<0||!std::isfinite(q.bank_scale))return std::nullopt;
 for(float x:q.steering_direction)if(!std::isfinite(x))return std::nullopt;
 Rounding rounding;const V &direction=q.prelimit_speed_squared<=c(0x3e4ccccc)?q.steering_direction:q.velocity;
 if(dot3(direction,direction)<=0)return q.quaternion;
 V d=norm3(direction);for(float &x:d)x=-x;V target{-d[1],d[0],0,add(1,d[2])};
 if(-d[2]>c(0x3f7ffcb9))target={0,c(0x3f7fffff),0,-c(0x333bbd2f)};else target=norm4(target);
 // A horizontal basis gives a zero cross product in 2590. Native ground
 // motion uses zero bank in that degenerate limit; IEEE interpreter output
 // is nonfinite there and cannot serve as an SPU extended-exponent oracle.
 float angle=q.bank_scale==0?0:mul(bank_angle(target),q.bank_scale);
 if(!std::isfinite(angle)||std::fabs(angle)>1e6f)return std::nullopt;
 V adjustment{0,0,sine(mul(angle,.5f)),sine(mul(angle,.5f),true)};target=product(target,adjustment);
 float a=ma(q.quaternion[1],target[1],mul(q.quaternion[0],target[0]));float b=ma(q.quaternion[3],target[3],mul(q.quaternion[2],target[2]));float dot=add(a,b);V prior=q.quaternion;if(dot<0){dot=-dot;for(float &x:prior)x=-x;}
 float t=mul(mul(q.elapsed,c(0x412fedde)),q.turn_rate);
 if(!std::isfinite(t)||t>1e6f)return std::nullopt;
 float old=sub(1,t),newweight=t,oldweight=old;
 if(dot<c(0x3f7fbe76)){float theta=acos_original(dot),denom=sine(theta),e=detail::reciprocal_estimate(denom),error=nm(denom,e,c(0x3f800001));float n=mul(sine(mul(t,theta)),e),o=mul(sine(mul(old,theta)),e);newweight=ma(error,n,n);oldweight=ma(error,o,o);}
 V result{};for(int i=0;i<4;i++)result[i]=ma(prior[i],oldweight,mul(target[i],newweight));result=norm4(result);for(float x:result)if(!std::isfinite(x))return std::nullopt;return result;
}
}
