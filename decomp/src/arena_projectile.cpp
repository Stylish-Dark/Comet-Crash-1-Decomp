#include "comet/arena_projectile.hpp"
#include "arena_spu_math.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
unsigned word(const ArenaEntitySlot&s,unsigned o){unsigned v=0;for(unsigned i=0;i<4;i++)v=(v<<8)|s[o+i];return v;}
float scalar(const ArenaEntitySlot&s,unsigned o){return std::bit_cast<float>(word(s,o));}
void put(ArenaEntitySlot&s,unsigned o,unsigned v){for(unsigned i=0;i<4;i++)s[o+i]=v>>(24-8*i);}
void fp(ArenaEntitySlot&s,unsigned o,float f){put(s,o,std::bit_cast<unsigned>(f));}
bool number(float f){return std::isfinite(f)&&std::fpclassify(f)!=FP_SUBNORMAL;}
struct Rounding {int previous=std::fegetround();Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO))throw std::runtime_error("projectile rounding unavailable");}~Rounding(){std::fesetround(previous);}};
float mul(float a,float b){volatile float r=a*b;return r;}
float add(float a,float b){volatile float r=a+b;return r;}
float c(unsigned u){return std::bit_cast<float>(u);}
float squared(const std::array<float,4>&v){return std::fma(v[2],v[2],std::fma(v[1],v[1],mul(v[0],v[0])));}
float root(float s){const float e=detail::inverse_root_estimate(s),r=mul(e,s);return std::fma(std::fma(-e,r,c(0x3f800001)),mul(r,.5f),r);}
float inv(float s){const float e=detail::inverse_root_estimate(s),r=mul(e,s);return std::fma(std::fma(-e,r,c(0x3f800001)),mul(e,.5f),e);}
std::array<float,4> rotation(std::array<float,4>d){
 if(-d[2]>c(0x3f7ffcb9))return {0,c(0x3f7fffff),0,-c(0x333bbd2f)};
 std::array<float,4> q{std::fma(-d[2],-0.f,std::fma(d[1],-1.f,std::fma(d[0],0.f,-0.f))),std::fma(-d[0],-1.f,std::fma(d[2],-0.f,std::fma(d[1],0.f,-0.f))),std::fma(-d[1],-0.f,std::fma(d[0],-0.f,std::fma(d[2],0.f,-0.f))),add(1,d[2])};
 std::array<float,4> out;for(unsigned i=0;i<4;i++){const unsigned j=(i+1)%4,k=(i+2)%4,l=(i+3)%4;const float a=std::fma(q[j],q[j],mul(q[i],q[i])),b=std::fma(q[l],q[l],mul(q[k],q[k]));out[i]=mul(q[i],inv(add(a,b)));}return out;
}
}
ArenaProjectileStep advance_arena_projectile(ArenaEntitySlot&s,float dt,const ArenaProjectileContact&contact,bool outside,const std::array<float,4>&acceleration){
 ArenaProjectileStep result;
 if(s[15]!=15&&s[15]!=3){result.status=ArenaProjectileStatus::Unsupported;return result;}
 if(!number(dt)||dt<=0||dt>.25f||contact.type>7)return result;
 for(unsigned o:{0u,4u,8u})if(!number(scalar(s,o))||std::fabs(scalar(s,o))>64||!number(scalar(s,0x40+o))||std::fabs(scalar(s,0x40+o))>1024)return result;
 if(contact.hit&&(contact.type==6||s[15]==3)&&(!number(scalar(s,0x58))||std::fabs(scalar(s,0x58))>1e6f||contact.target_index==0xffff))return result;
 if(s[15]==3)for(float f:acceleration)if(!number(f)||std::fabs(f)>1024)return result;
 for(const auto *v:{&contact.bounds_position,&contact.bounds_velocity,&contact.corrected_position})if(*v)for(unsigned i=0;i<3;i++)if(!number((**v)[i])||std::fabs((**v)[i])>(v==&contact.bounds_velocity?1024:320))return result;
 auto next=s;Rounding rounding;
 std::array<float,4> position;for(unsigned i=0;i<4;i++)position[i]=contact.bounds_position?(*contact.bounds_position)[i]:scalar(s,i*4);
 std::array<float,4> velocity;for(unsigned i=0;i<4;i++)velocity[i]=contact.bounds_velocity?(*contact.bounds_velocity)[i]:scalar(s,0x40+i*4);
 if(s[15]==3){
  put(next,0x80,word(s,0x80)+1);
  if(position[1]>1)for(unsigned i=0;i<4;i++)velocity[i]=std::fma(acceleration[i],dt,velocity[i]);
  const float sq=squared(velocity);if(!number(sq))return {};
  const float length=sq==0?0:root(sq);
  if(length>c(0x38d1b717)){
   const float den=-length,e=detail::reciprocal_estimate(den),error=std::fma(-den,e,c(0x3f800001));std::array<float,4>d;
   for(unsigned i=0;i<4;i++){const float base=mul(velocity[i],e);d[i]=std::fma(error,base,base);}
   const auto q=rotation(d);for(unsigned i=0;i<4;i++){if(!number(q[i]))return {};fp(next,0x20+i*4,q[i]);}
  }
 }

 std::array<float,3> predicted;
 for(unsigned i=0;i<3;i++){predicted[i]=std::fma(velocity[i],dt,position[i]);if(!number(predicted[i]))return {};}
 if(contact.corrected_position)for(unsigned i=0;i<3;i++)predicted[i]=(*contact.corrected_position)[i];
 if(contact.hit&&(contact.type==6||s[15]==3)){result.impact_message=true;result.target_index=contact.target_index;result.health_delta=std::bit_cast<float>(0x80000000u);for(unsigned i=0;i<4;i++)result.impulse[i]=mul(velocity[i],scalar(s,0x58));for(unsigned i=0;i<3;i++)if(!number(result.impulse[i]))return {};}
 // 6578 clears bit2 before either normal XYZ finalization or retirement.
 put(next,0x60,word(next,0x60)&~2u);
 if(contact.hit||outside)result.status=ArenaProjectileStatus::Retire;
 else {for(unsigned i=0;i<3;i++){fp(next,i*4,predicted[i]);fp(next,0x40+i*4,velocity[i]);}result.status=ArenaProjectileStatus::Active;}
 s=next;return result;
}
}
