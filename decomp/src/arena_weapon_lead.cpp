#include "comet/arena_weapon_lead.hpp"
#include "arena_spu_math.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp { namespace {
using V=std::array<float,4>;
float mul(float a,float b){volatile float r=a*b;return r;}
float add(float a,float b){volatile float r=a+b;return r;}
float sub(float a,float b){volatile float r=a-b;return r;}
float ma(float a,float b,float c){return std::fma(a,b,c);}
float nm(float a,float b,float c){return std::fma(-a,b,c);}
float dot(const V&a,const V&b){return ma(a[2],b[2],ma(a[1],b[1],mul(a[0],b[0])));}
float root(float s){if(s==0)return 0;float e=detail::inverse_root_estimate(s),v=mul(e,s);return ma(nm(e,v,std::bit_cast<float>(0x3f800001u)),mul(v,.5f),v);}
float divide(float n,float d){float e=detail::reciprocal_estimate(d),v=mul(n,e);return ma(nm(v,d,n),e,v);}
struct Rounding{int old=std::fegetround();Rounding(){if(old==-1||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("weapon lead rounding unavailable");}~Rounding(){std::fesetround(old);}};
}
std::optional<V> predict_arena_weapon_target(const V&origin,const V&target,const V&velocity,float speed){
 if(!std::isfinite(speed)||speed<=0||speed>10000)return std::nullopt;
 for(const V*v:{&origin,&target,&velocity}){if((*v)[3]!=0)return std::nullopt;for(float x:*v)if(!std::isfinite(x)||std::fabs(x)>100000)return std::nullopt;}
 Rounding rounding;V relative{};for(int i=0;i<4;i++)relative[i]=sub(target[i],origin[i]);
 float a=nm(speed,speed,dot(velocity,velocity)),b=dot(relative,velocity);b=add(b,b);
 float c=dot(relative,relative),disc=ma(b,b,mul(c,mul(a,-4.f)));
 if(disc<0)return target;
 float q=mul(ma(b<0?-1.f:1.f,root(disc),b),-.5f);
 constexpr float epsilon=std::bit_cast<float>(0x38d1b717u);
 if(std::fabs(a)<=epsilon||std::fabs(q)<=epsilon)return target;
 float t1=divide(c,q),t2=divide(q,a),t=((t1<0||t1>t2)&&t2>=0)?t2:t1;
 V result{};for(int i=0;i<4;i++){result[i]=ma(velocity[i],t,target[i]);if(!std::isfinite(result[i]))return std::nullopt;}
 return result;
}
}
