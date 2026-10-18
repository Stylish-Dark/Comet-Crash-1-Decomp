#include "comet/arena_cannon_source.hpp"
#include "comet/arena_entity_core.hpp"
#include "comet/arena_weapon_aim.hpp"
#include "comet/arena_weapon_lead.hpp"
#include "arena_spu_math.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
namespace comet::decomp {
namespace {
using V=std::array<float,4>;
unsigned word(const ArenaEntitySlot&s,unsigned o){unsigned v=0;for(unsigned i=0;i<4;i++)v=(v<<8)|s[o+i];return v;}
void word(ArenaEntitySlot&s,unsigned o,unsigned v){for(unsigned i=0;i<4;i++)s[o+i]=v>>(24-8*i);}
float scalar(const ArenaEntitySlot&s,unsigned o){return std::bit_cast<float>(word(s,o));}
void scalar(ArenaEntitySlot&s,unsigned o,float v){word(s,o,std::bit_cast<unsigned>(v));}
float bits(unsigned b){return std::bit_cast<float>(b);}
float mul(float a,float b){volatile float v=a*b;return v;}
float add(float a,float b){volatile float v=a+b;return v;}
float sub(float a,float b){volatile float v=a-b;return v;}
float square(const V&v){return std::fma(v[2],v[2],std::fma(v[1],v[1],mul(v[0],v[0])));}
float inverse(float s){const float i=detail::inverse_root_estimate(s);return std::fma(std::fma(-i,mul(i,s),bits(0x3f800001)),mul(i,.5f),i);}
float length(const V&v){const float s=square(v),i=detail::inverse_root_estimate(s),b=mul(i,s);return std::fma(std::fma(-i,b,bits(0x3f800001)),mul(b,.5f),b);}
bool finite(const V&v){for(float x:v)if(!std::isfinite(x))return false;return true;}
V position(const ArenaEntitySlot&s){V v{};for(unsigned i=0;i<3;i++)v[i]=scalar(s,4*i);return v;}
void vector(ArenaEntitySlot&s,unsigned o,const V&v){for(unsigned i=0;i<4;i++)scalar(s,o+4*i,v[i]);}
bool tower(const ArenaEntitySlot&s){return s[15]==21&&((s[0x33]>>2)&7)==4;}
struct Round {int old;~Round(){std::fesetround(old);}};
}
std::optional<ArenaCannonSourceResult> advance_arena_cannon_source(ArenaEntitySlot&s,const ArenaCannonSourceQuery&q){
 if(!tower(s)||!std::isfinite(q.elapsed)||q.elapsed<0||q.elapsed>3||!finite(position(s))||!finite(q.owner_base_center)||q.gathered_target==0xffff)return {};
 for(unsigned o:{0x78,0x5c,0xb8,0xbc,0xc0,0xc4,0xc8,0xd4,0xd8,0xe4,0xe8,0xec})if(!std::isfinite(scalar(s,o)))return {};
 const unsigned cached=(unsigned(s[0x1e])<<8)|s[0x1f];
 if(q.target){if(q.target->index!=cached||cached==0xffff||!finite(q.target->position)||!finite(q.target->velocity))return {};}
 else if(cached!=0xffff)return {}; // Cached validation failure/visibility rejection is caller-explicit.
 const int old=std::fegetround();if(old<0||std::fesetround(FE_TOWARDZERO))return {};Round round{old};
 ArenaEntitySlot next=s;ArenaCannonSourceResult result;V origin=position(s);origin[1]=std::fma(bits(0x3ec7ae14),scalar(s,0x78),origin[1]);
 float yaw_step=mul(q.elapsed,scalar(s,0xd4)),pitch_step=mul(q.elapsed,scalar(s,0xd8));
 if(scalar(s,0xe4)>scalar(s,0xbc)){yaw_step=mul(yaw_step,bits(0x3c23d70a));pitch_step=mul(pitch_step,bits(0x3c23d70a));}
 scalar(next,0xbc,add(scalar(s,0xbc),q.elapsed));scalar(next,0xc0,add(scalar(s,0xc0),q.elapsed));
 V target{},direction{};float distance=0;bool tracking=bool(q.target);
 if(tracking){target=q.target->position;target[1]=add(target[1],mul(mul(float(q.target->height_byte),bits(0x3c23d70a)),.5f));const auto led=predict_arena_weapon_target(origin,target,q.target->velocity,float(s[0xcd]));if(!led)return {};target=*led;for(unsigned i=0;i<4;i++)direction[i]=sub(target[i],origin[i]);distance=length(direction);if(!std::isfinite(distance)||distance>add(scalar(s,0x5c),1.f))return {};if(distance<=.25f)tracking=false;}
 if(!tracking){for(unsigned i=0;i<4;i++)direction[i]=sub(position(s)[i],q.owner_base_center[i]);yaw_step=mul(yaw_step,bits(0x3e4ccccc));pitch_step=mul(pitch_step,bits(0x3e4ccccc));}
 const float squared=square(direction);if(!std::isfinite(squared)||squared<0)return {};const float reciprocal=squared==0?0:inverse(squared);for(float &v:direction)v=mul(v,reciprocal);
 ArenaWeaponAimQuery aim;aim.direction=direction;aim.yaw=scalar(s,0xc4);aim.pitch=scalar(s,0xc8);aim.yaw_step=yaw_step;aim.pitch_step=pitch_step;aim.pitch_max=scalar(s,0xe8);aim.pitch_min=scalar(s,0xec);
 const auto oriented=prepare_arena_weapon_aim(aim);if(!oriented)return {};
 scalar(next,0xc4,oriented->yaw);scalar(next,0xc8,oriented->pitch);vector(next,0x20,oriented->source_orientation);
 if(tracking){const float tolerance=mul(add(mul(float(s[0xdd]),bits(0x3c23d70a)),mul(float(s[0xdd]),bits(0x3c23d70a))),bits(0x3fc90fda));
  if(scalar(next,0xbc)>scalar(s,0xb8)&&tolerance>oriented->remaining_angle){scalar(next,0xbc,0);result.fire=ArenaCannonFireIntent{origin,target,oriented->muzzle_orientation};}
 }
 if(cached==0xffff||scalar(next,0xc0)>.5f){result.gather_target=true;scalar(next,0xc0,0);const auto id=q.gathered_target.value_or(0xffff);next[0x1e]=id>>8;next[0x1f]=id;}
 s=next;return result;
}
bool initialize_arena_cannon_projectile(ArenaEntitySlot&p,const ArenaEntitySlot&s,const ArenaCannonFireIntent&i){
 if(!tower(s)||!finite(i.position)||!finite(i.target_position)||!finite(i.orientation)||!std::isfinite(scalar(s,0x3c)))return false;
 const int old=std::fegetround();if(old<0||std::fesetround(FE_TOWARDZERO))return false;Round round{old};
 V d;for(unsigned n=0;n<4;n++)d[n]=sub(i.target_position[n],i.position[n]);const float squared=square(d);if(squared<=0)return false;const float reciprocal=inverse(squared);for(float &v:d)v=mul(mul(v,reciprocal),float(s[0xcd]));
 ArenaEntityCoreParameters core;core.type=3;core.model_slot=27;core.owner=s[0x33]>>5;core.quantity=6;core.stage=1;core.position=i.position;core.scalar_58=1;core.scalar_3c=scalar(s,0x3c);core.scalar_38=core.scalar_64=50;
 initialize_arena_entity_core(std::span<std::uint8_t,128>(p.data(),128),core);vector(p,0x40,d);vector(p,0x20,i.orientation);p[0x6c]=s[0xc];p[0x6d]=s[0xd];word(p,0x10,126);word(p,0x80,0);word(p,0x84,45);return true;
}
} // namespace comet::decomp
