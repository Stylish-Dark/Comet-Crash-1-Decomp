#include "comet/arena_laser_beam.hpp"
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
 Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("beam rounding unavailable");}
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
unsigned word(const ArenaEntitySlot&s,unsigned o){unsigned v=0;for(unsigned i=0;i<4;i++)v=(v<<8)|s[o+i];return v;}
unsigned half(const ArenaEntitySlot&s,unsigned o){return (unsigned(s[o])<<8)|s[o+1];}
float scalar(const ArenaEntitySlot&s,unsigned o){return value(word(s,o));}
void put(ArenaEntitySlot&s,unsigned o,unsigned v){for(unsigned i=0;i<4;i++)s[o+i]=v>>(24-8*i);}
void ph(ArenaEntitySlot&s,unsigned o,unsigned v){s[o]=v>>8;s[o+1]=v;}
void fp(ArenaEntitySlot&s,unsigned o,float v){put(s,o,std::bit_cast<unsigned>(v));}
Vector vec(const ArenaEntitySlot&s,unsigned o){Vector v;for(unsigned i=0;i<4;i++)v[i]=scalar(s,o+4*i);return v;}
void pv(ArenaEntitySlot&s,unsigned o,const Vector&v){for(unsigned i=0;i<4;i++)fp(s,o+4*i,v[i]);}
bool number(float f){return std::isfinite(f)&&std::fpclassify(f)!=FP_SUBNORMAL;}
float draw(ArenaRandom&r){volatile float d=float(r.next());return divide(d,value(0x4e7fffff));}
Vector circle(ArenaRandom&r){const auto t=trig(std::fma(draw(r),value(0x40c90fda),0.f));return {t.sine,0,t.cosine,0};}
Vector norm(Vector v){const float s=squared(v);if(s==0)return v;const float e=detail::inverse_root_estimate(s),root=mul(e,s),f=std::fma(std::fma(-e,root,value(0x3f800001)),mul(e,.5f),e);for(float&x:v)x=mul(x,f);return v;}
}
ArenaLaserBeamStep advance_arena_laser_beam(ArenaEntitySlot &beam,const ArenaEntitySlot&parent,const ArenaEntitySlot*target,float dt,ArenaRandom&random,bool hit_effect_available){
 ArenaLaserBeamStep result;if(beam[15]!=5){result.status=ArenaLaserBeamStatus::Unsupported;return result;}
 if(!number(dt)||dt<=0||dt>.25f)return result;
 for(unsigned o:{0u,4u,8u,0x3cu,0xecu,0xf8u})if(!number(scalar(beam,o)))return result;
 for(unsigned o:{0u,4u,8u,0xb0u,0xc8u})if(!number(scalar(parent,o)))return result;
 if(target)for(unsigned o:{0u,4u,8u,0x40u,0x44u,0x48u})if(!number(scalar(*target,o)))return result;
 for(unsigned o:{0u,4u,8u}) {
  if(std::fabs(scalar(beam,o))>64 || std::fabs(scalar(parent,o))>64)return result;
  if(target && (std::fabs(scalar(*target,o))>64 || std::fabs(scalar(*target,0x40+o))>1024))return result;
 }
 for(unsigned o:{0x3cu,0xecu,0xf8u})if(std::fabs(scalar(beam,o))>1e6f)return result;
 if(std::fabs(scalar(parent,0xb0))>1e6f || std::fabs(scalar(parent,0xc8))>1e6f)return result;
 if(word(beam,0xe0)>=8)return result;
 auto next=beam;auto rng=random;Rounding rounding;
 if(parent[15]!=22 || !(word(parent,0x60)&0x01000000u) || scalar(parent,0xc8)>0){ph(next,0x1e,0xffff);beam=next;result.status=ArenaLaserBeamStatus::Retire;return result;}
 const auto parent_position=vec(parent,0);Vector origin=parent_position;origin[1]=add(origin[1],value(0x3ecccccc));
 Vector position=vec(beam,0);bool idle=true;
 const unsigned target_id=half(beam,0x1c);
 if(target && target_id!=0xffff && target->at(15)!=63){
  auto predicted=vec(*target,0);if((target->at(0x33)&28)==16)predicted[1]=add(predicted[1],mul(mul(float(target->at(14)),value(0x3c23d70a)),.5f));
  const auto velocity=vec(*target,0x40);Vector offset;for(unsigned i=0;i<4;i++){predicted[i]=std::fma(velocity[i],dt,predicted[i]);offset[i]=sub(parent_position[i],predicted[i]);}
  const float sq=squared(offset);if(!number(sq))return ArenaLaserBeamStep{};
  const float distance=sq==0?0:length(sq);
  if(2.5f>distance){idle=false;position=predicted;pv(next,0xd0,{});result.target_index=half(*target,0xc);result.health_delta=-mul(dt,scalar(beam,0x3c));
   if(target->at(15)==46){const float timer=add(scalar(beam,0xf8),dt);fp(next,0xf8,timer);if(timer>value(0x3e199999)){result.parent_command_increment=1;result.extra_health_delta=-1000.f;}}
  }
 }
 put(next,0xe4,static_cast<unsigned>(static_cast<std::int32_t>(static_cast<std::int16_t>(target_id))));
 // 1E880: one successful particle uses random direction (two draws),
 // then three scalar draws, before the beam jitter direction.
 if(!idle && dt>value(0x3a83126e) && hit_effect_available)
  for(unsigned i=0;i<5;i++)rng.next();
 Vector jitter=arena_ground_random_direction(rng);jitter[1]=add(jitter[1],.5f);jitter=norm(jitter);
 if(!idle)next[0xfc]=0;
 else {
  fp(next,0xf8,0);const float timer=sub(scalar(next,0xec),dt);fp(next,0xec,timer);
  if(next[0xfc]!=1 || timer<0){fp(next,0xec,std::fma(draw(rng),value(0x3f4ccccc),value(0x3dcccccc)));const auto d=circle(rng);for(unsigned i=0;i<4;i++)position[i]=std::fma(d[i],value(0x3ee66666),parent_position[i]);next[0xfc]=1;}
  pv(next,0xd0,{});
 }
 const unsigned ordinal=word(next,0xe0);
 ph(next,0x1c,half(parent,0x90+ordinal*2));put(next,0x3c,word(parent,0xb0));next[0x33]=(next[0x33]&~3u)|(parent[0x33]&3u);
 Vector delta,mid;for(unsigned i=0;i<4;i++){delta[i]=sub(origin[i],position[i]);mid[i]=mul(add(origin[i],position[i]),.5f);}
 const float sq=squared(delta);if(!number(sq))return ArenaLaserBeamStep{};
 const float distance=sq==0?0:length(sq),extent=mul(distance,value(0x3f733333)),limit=mul(extent,.5f);
 pv(next,0x80,origin);pv(next,0x90,origin);pv(next,0xb0,position);pv(next,0xc0,position);
 Vector bend=vec(next,0xd0);for(unsigned i=0;i<4;i++)bend[i]=std::fma(mul(jitter[i],extent),value(0x3dcccccc),bend[i]);
 const float bs=squared(bend);if(!number(bs))return ArenaLaserBeamStep{};
 const float bl=bs==0?0:length(bs);if(bl>limit){const float factor=divide(extent,bl);for(float&v:bend)v=mul(v,factor);}
 pv(next,0xd0,bend);for(unsigned i=0;i<4;i++)mid[i]=add(mid[i],bend[i]);pv(next,0xa0,mid);
 for(unsigned i=0;i<3;i++)fp(next,i*4,position[i]);
 put(next,0x60,word(next,0x60)&~2u);
 for(unsigned o=0x80;o<0xe0;o+=4)if((o&15)!=12&&!number(scalar(next,o)))return ArenaLaserBeamStep{};
 if(!number(scalar(next,0xec))||!number(scalar(next,0xf8))||!number(result.health_delta)||!number(result.extra_health_delta))return ArenaLaserBeamStep{};
 beam=next;random=rng;result.status=ArenaLaserBeamStatus::Active;return result;
}
}
