#include "comet/arena_player_peer.hpp"
#include "comet/arena_ground_pair.hpp"
#include "comet/arena_ground_contact.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {namespace {
using V=std::array<float,4>;
unsigned word(const auto& b,unsigned o){return unsigned(b[o])<<24|unsigned(b[o+1])<<16|unsigned(b[o+2])<<8|b[o+3];}
unsigned half(const auto& b,unsigned o){return unsigned(b[o])<<8|b[o+1];}
void put(auto& b,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(24-8*i));}
float bits(unsigned v){return std::bit_cast<float>(v);}
float scalar(const auto& b,unsigned o){return bits(word(b,o));}
void fp(auto& b,unsigned o,float v){put(b,o,std::bit_cast<unsigned>(v));}
V vector(const auto& b,unsigned o){V v;for(unsigned i=0;i<4;++i)v[i]=scalar(b,o+4*i);return v;}
void store(auto& b,unsigned o,const V& v){for(unsigned i=0;i<4;++i)fp(b,o+4*i,v[i]);}
bool number(float v){return std::isfinite(v)&&std::fpclassify(v)!=FP_SUBNORMAL&&std::abs(v)<=1e6f;}
float mul(float a,float b){volatile float v=a*b;return v;}
float sub(float a,float b){volatile float v=a-b;return v;}
float add(float a,float b){volatile float v=a+b;return v;}
float dot(const V& a,const V& b){return std::fma(a[2],b[2],std::fma(a[1],b[1],mul(a[0],b[0])));}
struct Rounding{int previous=std::fegetround();Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("peer rounding unavailable");}~Rounding(){std::fesetround(previous);}};
}
ArenaPlayerPeerResult prepare_arena_player_peer_contact(ArenaEntityWorkContext& context,
 std::span<const std::uint8_t,0x2900> snapshot,ArenaRandom& random){
 ArenaPlayerPeerResult result;
 if(context[15]!=1||word(context,0x500)!=1)return result;
 auto next=context;auto rng=random;result.peer_index=std::uint16_t(half(next,0x1c));
 if(next[0x10f]>=62){next[0x1c]=next[0x1d]=255;context=next;result.advanced=true;return result;}
 if(result.peer_index==65535||result.peer_index==half(next,0x6c)){result.advanced=true;return result;}
 const unsigned owner=next[0x133]>>5;
 const bool hostile=word(next,0x530)!=half(snapshot,0x2720+2*owner)&&!(word(next,0x160)&0x20000u);
 if(word(next,0x510)==5&&next[0x10f]==45&&hostile&&next[0x1bb])return result;
 ArenaGroundPairQuery q;q.position=vector(next,0x480);q.peer_position=vector(next,0x100);
 q.radius=scalar(next,0x4e0);q.own_mass=scalar(next,0x58);q.peer_mass=scalar(next,0x158);
 q.peer_radius=next[0x10e];q.index=std::uint16_t(word(next,0x4f0));q.peer_index=std::uint16_t(half(next,0x10c));
 for(float f:{q.radius,q.own_mass,q.peer_mass})if(!number(f)||f<0)return result;
 for(const auto& v:{q.position,q.peer_position,vector(next,0x4a0),vector(next,0x140),vector(next,0x4d0)})for(unsigned i=0;i<4;++i)if(i<3?!number(v[i]):!std::isfinite(v[i]))return result;
 // Actual source self-link6C is checked above; geometry's distinct-index guard
 // excludes inconsistent fetched self records rather than inventing a response.
 if(q.index==q.peer_index||(q.position[0]==q.peer_position[0]&&q.position[1]==q.peer_position[1]&&q.position[2]==q.peer_position[2]))return result;
 result.peer_index=q.peer_index;
 Rounding rounding;q.jitter=arena_ground_random_direction(rng);
 const auto pair=prepare_arena_ground_pair(q);if(!pair)return result;
 // The fourth position lane is metadata retained by the source vector math.
 auto position=pair->position;
 store(next,0x480,position);fp(next,0x570,hostile?100.f:pair->gap);store(next,0x580,pair->normal);
 if(pair->overlapping){
  const bool category_one=((next[0x133]>>2)&7u)==1;
  next[0x591]=hostile&&!category_one;next[0x592]=!hostile&&!category_one;
  ArenaGroundContactQuery contact;contact.position=position;contact.velocity=vector(next,0x4a0);
  contact.peer_velocity=vector(next,0x140);contact.normal=pair->normal;contact.accumulated_impulse=vector(next,0x4d0);
  contact.penetration=-pair->gap;contact.response_share=pair->response_share;
  contact.restitution_percent=next[0x7d];contact.movable=(word(next,0x60)&0x4000u)!=0;
  const auto response=resolve_arena_ground_contact(contact);if(!response)return result;
  store(next,0x480,response->position);store(next,0x4a0,response->velocity);store(next,0x4d0,response->accumulated_impulse);
  //7510 emits the peer impulse only if velocity response was actually taken.
  //8388 compares peer1C with own4F0; it does not compare peer identity.
  // A zero impulse can still be a source call. Separating-motion gates are
  // computed by the shared contact response, whose new flag records that gate.
  if(response->velocity_response&&half(next,0x11c)!=word(next,0x4f0)){
   V relative,negative;for(unsigned i=0;i<4;++i){relative[i]=sub(response->velocity[i],contact.peer_velocity[i]);negative[i]=-contact.normal[i];}
   const auto projection=dot(relative,negative),share=sub(1.f,contact.response_share);
   for(unsigned i=0;i<4;++i)result.impulse[i]=mul(mul(negative[i],mul(share,projection)),q.peer_mass);
   result.outgoing_impulse=true;
  }
  if(response->impact_strength>0){
   ArenaParticleEmission effect;effect.position=response->position;
   for(unsigned i=0;i<4;++i)effect.position[i]=std::fma(contact.normal[i],q.radius,effect.position[i]);
   effect.position[1]=add(effect.position[1],bits(0x3ca3d70a));
   effect.velocity=contact.normal;effect.speed=3;effect.lifetime_min=bits(0x3e4ccccc);effect.lifetime_max=bits(0x3e999999);
   const auto count=word(next,0x5a0);effect.count=std::uint32_t(mul(response->impact_strength,float(count)))+(count?1:0);
   put(effect.configuration,0,0x3d0b4395);put(effect.configuration,4,0xc0000000);
   effect.configuration[8]=48;effect.configuration[9]=effect.configuration[10]=effect.configuration[11]=1;
   result.particles=effect;
  }
  result.overlapping=true;
 }
 context=next;random=rng;result.advanced=true;return result;
}
ArenaAccumulatorStatus submit_arena_peer_impulse(ArenaAccumulatorRecord& scratch,
 std::uint32_t ea,const V& impulse,const ArenaAccumulatorTransport& transport){
 if(!transport.read_reserved||!transport.write_conditional)return ArenaAccumulatorStatus::MissingTransport;
 for(unsigned i=0;i<4;++i)if(i<3?!number(impulse[i]):!std::isfinite(impulse[i]))return ArenaAccumulatorStatus::Invalid;
 Rounding rounding;
 for(;;){
  const auto loaded=transport.read_reserved(ea);if(!loaded)return ArenaAccumulatorStatus::MissingTransport;
  auto next=*loaded;
  for(unsigned i=0;i<4;++i){
   const float health=scalar(next,0x10+4*i),old=scalar(next,0x40+4*i);
   if(!number(health)||(i<3?!number(old):!std::isfinite(old)))return ArenaAccumulatorStatus::Invalid;
   const float value=add(old,impulse[i]);if(i<3?!number(value):!std::isfinite(value))return ArenaAccumulatorStatus::Invalid;
   fp(next,0x10+4*i,add(health,0));fp(next,0x40+4*i,value);
  }
  const auto status=transport.write_conditional(ea,next);
  if(status==ArenaAccumulatorWrite::Unavailable)return ArenaAccumulatorStatus::MissingTransport;
  if(status==ArenaAccumulatorWrite::Committed){scratch=next;return ArenaAccumulatorStatus::Cached;}
 }
}
}
