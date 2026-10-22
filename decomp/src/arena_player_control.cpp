#include "comet/arena_player_control.hpp"
#include "comet/arena_entity_core.hpp"
#include "comet/arena_ground_contact.hpp"
#include "comet/arena_craft_facing.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
using V=std::array<float,4>;
unsigned word(const auto& b,unsigned o){return unsigned(b[o])<<24|unsigned(b[o+1])<<16|unsigned(b[o+2])<<8|b[o+3];}
unsigned half(const auto& b,unsigned o){return unsigned(b[o])<<8|b[o+1];}
float scalar(const auto& b,unsigned o){return std::bit_cast<float>(word(b,o));}
void put(auto& b,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(24-8*i));}
void ph(auto& b,unsigned o,unsigned v){b[o]=std::uint8_t(v>>8);b[o+1]=std::uint8_t(v);}
void fp(auto& b,unsigned o,float v){put(b,o,std::bit_cast<unsigned>(v));}
struct Numeric{};
float check(float v){if(!std::isfinite(v)||std::fpclassify(v)==FP_SUBNORMAL||std::abs(v)>1000000)throw Numeric{};return v;}
float mul(float a,float b){volatile float r=a*b;return check(r);}
float add(float a,float b){volatile float r=a+b;return check(r);}
float sub(float a,float b){volatile float r=a-b;return check(r);}
float ma(float a,float b,float c){return check(std::fma(a,b,c));}
V vector(const auto& b,unsigned o){V v;for(unsigned i=0;i<4;++i)v[i]=check(scalar(b,o+4*i));return v;}
void store(auto& b,unsigned o,const V& v){for(unsigned i=0;i<4;++i)fp(b,o+4*i,v[i]);}
struct Rounding {
 int old=std::fegetround();
 Rounding(){if(old<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("player control rounding unavailable");}
 ~Rounding(){std::fesetround(old);}
};
ArenaPlayerControlResult blocked(ArenaPlayerControlStatus s,unsigned pc){return {s,pc};}
// 3238 ->2DF8, then3358..339C (addresses below are job local).
void pending_descriptor(ArenaEntitySlot& descriptor,const ArenaEntityWorkContext& c){
 ArenaEntityCoreParameters p;p.type=7;p.model_slot=33;p.owner=std::uint8_t(half(c,0x542));
 p.quantity=50;p.scalar_58=100;p.scalar_38=p.scalar_64=100;
 initialize_arena_entity_core(std::span<std::uint8_t,256>(descriptor).first<128>(),p);
 descriptor[0x83]=1;ph(descriptor,0x1c,word(c,0x4f0));put(descriptor,0x54,0);put(descriptor,0x78,0);
}
// 11718..117EC preserves all unassigned reused descriptor bytes.
ArenaCounterEmissionRequest owner_request(ArenaEmissionDescriptor descriptor,
 unsigned owner,std::uint16_t previous){
 fp(descriptor,0,0);fp(descriptor,4,-5);fp(descriptor,8,0);
 descriptor[14]=0;descriptor[15]=109;
 std::copy_n(descriptor.begin(),16,descriptor.begin()+0x20);
 ph(descriptor,12,0);put(descriptor,0x10,16);
 ph(descriptor,0x30,previous);descriptor[0x32]=descriptor[0x33]=1;
 put(descriptor,0x34,0);fp(descriptor,0x38,-1);fp(descriptor,0x3c,-1);
 put(descriptor,0x40,0);put(descriptor,0x44,0x3e20d97b);
 put(descriptor,0x50,owner);descriptor[0x54]=0;
 return {descriptor,1,0};
}
}
ArenaPlayerControlResult advance_arena_player_control(ArenaEntityWorkContext& context,
 std::span<const std::uint8_t,0x2900> snapshot,float elapsed,
 ArenaPlayerControlScratch* scratch,const ArenaPlayerPendingTransport& pending,
 const ArenaCounterEmissionSink& emitter){
 using S=ArenaPlayerControlStatus;
 if(context[15]!=1||word(context,0x500)!=1||word(context,0x5d0)!=0x80||context[0xc6]>=4||
    half(context,0x542)!=context[0xc6]||!std::isfinite(elapsed)||elapsed<0||elapsed>.25f||
    !word(snapshot,0x28d8)||word(snapshot,0x28d8)>24||
    !word(snapshot,0x28dc)||word(snapshot,0x28dc)>24)return {};
 const unsigned owner=context[0xc6],base=0x2550u+64u*owner;
 const bool pending_needed=(word(context,0x60)&0x8000)!=0;
 if(pending_needed&&(!scratch||!pending.reserve||!pending.publish))return blocked(S::NeedsPendingQueue,0x103cc);
 // 7E40's single-target sentinel path is recovered here; real peers remain
 // explicit. Kind>=62 also clears the target halfword, but is not required
 // when the supplied target already isFFFF.
 if((context[0x33]&0x1c)&&half(context,0x1c)!=65535)return blocked(S::NeedsPeerContact,0x1162c);
 if(snapshot[base+0x29]==2)return blocked(S::NeedsAlternateControl,0x11ca8);
 if(half(context,0xb8)!=65535)return blocked(S::NeedsAttachment,0x10610);
 if(context[0xbf])return blocked(S::NeedsTargetFacing,0x106b0);
 if(word(snapshot,0x28d4))return blocked(S::NeedsBanking,0x10b60);
 auto next=context;Rounding rounding;bool owner_emission=false;
 ArenaCounterEmissionRequest request;
 try {
  const auto x=check(scalar(snapshot,base+0x10)),z=check(scalar(snapshot,base+0x14));
  if(std::abs(x)>1||std::abs(z)>1)return {};
  // The inherited aim proof excludes aligned/near-aligned Z. Reject that
  // domain here instead of introducing the preview's finite analytic limit.
  if((x!=0||z!=0)&&std::abs(x)<mul(std::abs(z),.001f))return blocked(S::NeedsNumericRecovery,0x1a118);
  put(next,0x5a0,check(scalar(next,0x74))>0?6:0);
  store(next,0x480,vector(next,0xe0));store(next,0x20,vector(next,0xf0));next[0xbd]=0;
  auto position=vector(next,0x480),velocity=vector(next,0x4a0);
  for(unsigned i=0;i<4;++i)position[i]=ma(velocity[i],elapsed,position[i]);
  put(next,0x5a0,0); // 10538
  ArenaGroundBoundsQuery bounds;bounds.position=position;bounds.velocity=velocity;
  bounds.accumulated_impulse=vector(next,0x4d0);
  bounds.max_x=float(word(snapshot,0x28d8));bounds.max_z=float(word(snapshot,0x28dc));bounds.max_y=2;
  bounds.radius=check(scalar(next,0x4e0));bounds.lateral_radius=check(scalar(next,0x4e4));
  bounds.restitution_percent=next[0x7d];bounds.movable=(word(next,0x60)&0x4000)!=0;
  bounds.floor_uses_lateral_radius=(word(next,0x60)&0x02000000u)!=0;
  auto contained=contain_arena_ground_unit(bounds);if(!contained)throw Numeric{};
  if(contained->energetic_impact)return blocked(S::NeedsContactEffects,0x1e880);
  position=contained->position;velocity=contained->velocity;
  auto impulse=contained->accumulated_impulse;auto consumed=vector(next,0x340);
  for(unsigned i=0;i<4;++i)impulse[i]=add(impulse[i],consumed[i]);
  store(next,0x4d0,impulse);
  next[0xc7]=ma(impulse[2],impulse[2],ma(impulse[1],impulse[1],mul(impulse[0],impulse[0])))>3?1:0;
  const V target{mul(x,40),0,mul(z,40),0};
  for(unsigned i=0;i<4;++i)velocity[i]=ma(sub(target[i],velocity[i]),1,velocity[i]);
  velocity[1]=0;position[1]=1;store(next,0x480,position);store(next,0x4a0,velocity);
  // 11640 sentinel branch: reset action progress and staged collision mask.
  put(next,0x5a0,0);fp(next,0x78,std::bit_cast<float>(0x3c23d70au));next[0x33]&=0xe3;
  ArenaCraftFacingState facing;facing.yaw=check(scalar(next,0xdc));facing.pending_orientation=vector(next,0xf0);
  auto visible=advance_arena_craft_facing(facing,x,z,elapsed,false,false);
  fp(next,0xdc,facing.yaw);store(next,0x20,visible);store(next,0xf0,facing.pending_orientation);
  const auto action=check(scalar(snapshot,base+0x34));
  if(action>0&&!next[0xbc])return blocked(S::NeedsCarry,next[0xbb]?0x11808:0x107b0);
  // 113E0 clears attachment/target-facing state; stack-only rotated points are
  // dead in this zero-mode branch and have no outgoing observable writes.
  ph(next,0x1e,65535);next[0xbe]=next[0xbf]=0;next[0xbb]=1;
  next[0x590]=std::uint8_t((!(check(scalar(next,0x38))>0))&(next[0xbc]^1u));
  if(next[0x590])return blocked(S::NeedsDeath,0x12288);
  const auto target_owner=word(snapshot,0x2790+4*owner);
  const auto previous=std::bit_cast<std::int16_t>(std::uint16_t(half(next,0xc4)));
  if(target_owner!=0xffffffffu&&std::uint32_t(std::int32_t(previous))==target_owner&&word(snapshot,0x28cc)!=1){
   const float old=check(scalar(next,0xb4)),updated=add(old,elapsed);fp(next,0xb4,updated);
   owner_emission=!(old>.5f)&&updated>.5f;
   if(owner_emission){if(!scratch||!emitter)return blocked(S::NeedsOwnerEmission,0x117ec);
    request=owner_request(scratch->owner_effect,owner,std::uint16_t(previous));}
  }else put(next,0xb4,0);
  ph(next,0xc4,target_owner);
  // 112E4/113A0 exchange old visible position and next controller position,
  // just as11300/1139C exchange the delayed quaternion.
  store(next,0x480,vector(context,0xe0));store(next,0xe0,position);
 }catch(const Numeric&){return blocked(S::NeedsNumericRecovery,0x10290);}
 catch(const std::invalid_argument&){return blocked(S::NeedsNumericRecovery,0x1a118);}
 // All rejecting computation precedes external mutations. Queue callbacks may
 // only observe their supplied descriptor, so deferred pure work is unobservable.
 auto staged=scratch?*scratch:ArenaPlayerControlScratch{};
 if(pending_needed){
  pending_descriptor(staged.pending_entity,context);
  const auto index=pending.reserve(staged.pending_entity);
  if(index!=-1){
   if(index<0||index>65535)throw std::logic_error("invalid pending entity identity");
   ph(staged.pending_entity,12,unsigned(index));put(next,0x60,word(next,0x60)&~0x8000u);
   pending.publish(staged.pending_entity);
  }
 }
 if(owner_emission){emitter(request);staged.owner_effect=request.descriptor;}
 if(scratch)*scratch=staged;context=next;return {S::Advanced,0};
}
}
