#include "comet/arena_player_control.hpp"
#include "comet/arena_player_peer.hpp"
#include "comet/arena_entity_core.hpp"
#include "comet/arena_entity_finalization.hpp"
#include "comet/arena_ground_contact.hpp"
#include "comet/arena_craft_facing.hpp"
#include "comet/arena_weapon_aim.hpp"
#include "arena_spu_math.hpp"
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
float lane_check(float v,unsigned i){if(i!=3)return check(v);if(!std::isfinite(v)||std::abs(v)>1e6f)throw Numeric{};return v;}
float lane_mul(float a,float b,unsigned i){volatile float v=a*b;return lane_check(v,i);}
float lane_add(float a,float b,unsigned i){volatile float v=a+b;return lane_check(v,i);}
float lane_sub(float a,float b,unsigned i){volatile float v=a-b;return lane_check(v,i);}
float lane_ma(float a,float b,float c,unsigned i){return lane_check(std::fma(a,b,c),i);}
V vector(const auto& b,unsigned o){V v;for(unsigned i=0;i<4;++i)v[i]=lane_check(scalar(b,o+4*i),i);return v;}
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
// 1080C..10910: quaternion rotation of the current forward vector.
float nm(float a,float b,float c){return check(std::fma(-a,b,c));}
float dot(const V& a,const V& b){return ma(a[2],b[2],ma(a[1],b[1],mul(a[0],b[0])));}
float root(float squared){
 const float estimate=detail::inverse_root_estimate(squared),v=mul(estimate,squared);
 return ma(nm(estimate,v,std::bit_cast<float>(0x3f800001u)),mul(v,.5f),v);
}
float inverse(float squared){
 const float estimate=detail::inverse_root_estimate(squared);
 return check(std::fma(nm(estimate,mul(estimate,squared),std::bit_cast<float>(0x3f800001u)),estimate*.5f,estimate));
}
V difference(const V& a,const V& b){V v{};for(unsigned i=0;i<3;++i)v[i]=sub(a[i],b[i]);return v;}
V rotate_point(const V& q,const V& point){
 const float projection=add(ma(point[1],q[1],mul(q[0],point[0])),mul(q[2],point[2]));
 V intermediate{},result{};
 for(unsigned i=0;i<4;++i){const auto j=i==3?3:(i+1)%3,k=i==3?3:(i+2)%3;
  intermediate[i]=nm(q[k],point[j],ma(q[j],point[k],mul(q[3],point[i])));}
 for(unsigned i=0;i<4;++i){const auto j=i==3?3:(i+1)%3,k=i==3?3:(i+2)%3;
  result[i]=nm(q[k],intermediate[j],ma(q[j],intermediate[k],ma(q[3],intermediate[i],mul(projection,q[i]))));}
 return result;
}
// 1F560's first return lane: distance from point to the gripper segment.
// Retain the source estimate/refinement and endpoint test operation order.
float segment_distance(const V& point,const V& a,const V& b){
 const auto delta=difference(b,a),from_a=difference(point,a);
 V direction=delta;const float factor=inverse(dot(delta,delta));
 for(auto& x:direction)x=mul(x,factor);
 const float projection=dot(from_a,direction);V nearest;
 for(unsigned i=0;i<4;++i)nearest[i]=ma(direction[i],projection,a[i]);
 const auto perpendicular=difference(point,nearest),left=difference(nearest,a),right=difference(b,nearest);
 const float perpendicular_distance=root(dot(perpendicular,perpendicular));
 const float l=dot(left,left),r=dot(right,right),epsilon=std::bit_cast<float>(0x38d1b717u);
 if(epsilon>l||epsilon>r)return perpendicular_distance;
 V ln=left,rn=right;const float li=inverse(l),ri=inverse(r);
 for(unsigned i=0;i<4;++i){ln[i]=mul(ln[i],li);rn[i]=mul(rn[i],ri);}
 if(dot(ln,rn)>std::bit_cast<float>(0x3f7ff972u))return perpendicular_distance;
 return root(dot(r>l?from_a:difference(point,b),r>l?from_a:difference(point,b)));
}
// Source2400: quaternion taking positive Z toward the supplied direction.
// The near-negative-Z path uses the original finite 180-degree limit.
V direction_orientation(const V& direction){
 const float squared=dot(direction,direction);
 if(std::bit_cast<float>(0x38d1b717u)>squared)return {0,0,0,1};
 const float dot_negative_z=ma(direction[2],-1.f,ma(direction[1],0.f,mul(direction[0],0.f)));
 if(dot_negative_z>std::bit_cast<float>(0x3f7ffcb9u))
  return {0.f,std::bit_cast<float>(0x3f7fffffu),0.f,std::bit_cast<float>(0xb33bbd2fu)};
 // 24B4..24FC cross(direction,-Z), scalar1+Z. Keep signed-zero
 // operands from the original constants rather than negating scalar values.
 V cross{};
 const V yzx{direction[1],direction[2],direction[0],0},
         zxy{direction[2],direction[0],direction[1],0},
         minus_x{-1.f,-0.f,-0.f,0},minus_y{-0.f,-1.f,-0.f,0},
         minus_z{-0.f,-0.f,-1.f,0};
 for(unsigned i=0;i<4;++i)
  cross[i]=nm(zxy[i],minus_y[i],ma(yzx[i],minus_x[i],ma(direction[i],0.f,mul(0.f,minus_z[i]))));
 cross[3]=add(direction[2],1.f);
 // 2500..252C retains a distinct cyclic sum per lane. Reassociating all
 // lanes to one scalar norm changes Y/W by one ULP for ordinary inputs.
 V normalized{};
 for(unsigned i=0;i<4;++i){
  const auto j=(i+1)%4,k=(i+2)%4,l=(i+3)%4;
  const float norm=add(ma(cross[j],cross[j],mul(cross[i],cross[i])),ma(cross[l],cross[l],mul(cross[k],cross[k])));
  if(!(norm>0))throw Numeric{};
  const float estimate=detail::inverse_root_estimate(norm);
  const float refined=ma(nm(estimate,mul(estimate,norm),std::bit_cast<float>(0x3f800001u)),mul(estimate,.5f),estimate);
  normalized[i]=mul(cross[i],refined);
 }
 cross=normalized;
 return cross;
}
V normalize(const V& v){V r=v;const float factor=inverse(dot(v,v));for(unsigned i=0;i<4;++i)r[i]=lane_mul(r[i],factor,i);return r;}
float divide(float a,float b){
 if(b==0)throw Numeric{};
 const float estimate=check(detail::reciprocal_estimate(b)),initial=mul(a,estimate);
 return ma(nm(initial,b,a),estimate,initial);
}
V point(const V& p,const V& q,const V& offset,float scale){
 V local{};for(unsigned i=0;i<4;++i)local[i]=mul(offset[i],scale);
 const auto rotated=rotate_point(q,local);V result{};
 for(unsigned i=0;i<4;++i)result[i]=lane_add(p[i],rotated[i],i);
 return result;
}
float bits(unsigned value){return std::bit_cast<float>(value);}
// Source2DF8 core and10290's per-proxy continuation. Reused extension bytes
// remain owned by scratch. Model1 and40 are two parts of each interaction arm.
void proxy_descriptor(ArenaEntitySlot& descriptor,const ArenaEntityWorkContext& c,
 const V& position,const V& orientation,unsigned model,unsigned previous,float progress){
 ArenaEntityCoreParameters p;p.type=13;p.model_slot=std::uint8_t(model);p.owner=std::uint8_t(half(c,0x542));
 p.quantity=100;p.scalar_58=0;p.scalar_38=p.scalar_64=100;p.position=position;
 initialize_arena_entity_core(std::span<std::uint8_t,256>(descriptor).first<128>(),p);
 store(descriptor,0x20,orientation);ph(descriptor,0x0c,previous);fp(descriptor,0x78,progress);
}
// Source180A0, craft call sites1105C/1108C/111D4/11204. All four pass r6=r7=0.
// Live slots allocate only onFFFF. Exhaustion leavesFFFF and publishes nothing;
// existing slots publish without reserving. The source's final halfword store
// follows publication, even when a fresh allocation failed. Dead r7=0 calls
// rebuild the scratch as a3400 tombstone, publish only an existing identity,
// and clear the link; the r7!=0 kind62 branch is outside these callsites.
void publish_proxy(ArenaEntityWorkContext& c,unsigned link,ArenaEntitySlot& descriptor,
 const ArenaPlayerPendingTransport& pending,unsigned frame){
 auto index=half(c,link);
 if(!(scalar(c,0x38)>0)){
  // r7=0 chooses182C0 ->3400, preserving the proxy model after construction.
  const auto model=descriptor[0x6f];initialize_arena_entity_tombstone(descriptor,frame);descriptor[0x6f]=model;
  if(index!=65535)pending.publish(descriptor);
  ph(c,link,65535);return;
 }
 if(index==65535){
  const auto allocated=pending.reserve(descriptor);
  if(allocated!=-1){if(allocated<0||allocated>65534)throw std::logic_error("invalid interaction proxy identity");index=unsigned(allocated);}
 }
 if(index!=65535){ph(descriptor,12,index);pending.publish(descriptor);}
 ph(c,link,index);
}
// 12A70/11820: two selectors for left slot, then two for right slot.
ArenaCounterEmissionRequest action_visual_request(ArenaEmissionDescriptor descriptor,
 const ArenaEntityWorkContext& context,unsigned slot,bool second_target){
 fp(descriptor,0,-100);fp(descriptor,4,-100);fp(descriptor,8,-100);
 descriptor[15]=112;descriptor[14]=0;
 std::copy_n(descriptor.begin(),16,descriptor.begin()+0x20);ph(descriptor,12,0);put(descriptor,0x10,18);
 fp(descriptor,0x30,second_target?-1.f:1.f);put(descriptor,0x3c,0);
 ph(descriptor,0x40,slot);ph(descriptor,0x42,21);
 ph(descriptor,0x44,half(context,second_target?0xca:0xc8));ph(descriptor,0x46,(context[0x33]>>5)&7);
 descriptor[0x48]=1;return {descriptor,1,0};
}
}
ArenaPlayerControlResult advance_arena_player_control(ArenaEntityWorkContext& context,
 std::span<const std::uint8_t,0x2900> snapshot,float elapsed,
 ArenaPlayerControlScratch* scratch,const ArenaPlayerPendingTransport& pending,
 const ArenaCounterEmissionSink& emitter,const ArenaPlayerParticleTransport& particles,const ArenaAudioEventSink& audio,const ArenaAccumulatorTransport& peer_accumulator){
 using S=ArenaPlayerControlStatus;
 if(context[15]!=1||word(context,0x500)!=1||word(context,0x5d0)!=0x80||context[0xc6]>=4||
    half(context,0x542)!=context[0xc6]||!std::isfinite(elapsed)||elapsed<0||elapsed>.25f||
    !word(snapshot,0x28d8)||word(snapshot,0x28d8)>24||
    !word(snapshot,0x28dc)||word(snapshot,0x28dc)>24)return {};
 const unsigned owner=context[0xc6],base=0x2550u+64u*owner;
 const bool pending_needed=(word(context,0x60)&0x8000)!=0;
 if(pending_needed&&(!scratch||!pending.reserve||!pending.publish))return blocked(S::NeedsPendingQueue,0x103cc);
 // Original11618 single-target peer call. The active geometry path requires
 // source RNG ownership; sentinel, stale-kind and ignored links do not draw.
 const bool peer_needed=(context[0x33]&0x1c)&&half(context,0x1c)!=65535;
 if(peer_needed&&context[0x10f]<62&&half(context,0x1c)!=half(context,0x6c)&&!particles.random)return blocked(S::NeedsPeerContact,0x1162c);
 const bool automatic=snapshot[base+0x29]==2;
 const bool attached=half(context,0xb8)!=65535;
 if(attached&&(half(context,0x60c)!=half(context,0xb8)||context[0x60f]==0||context[0x60f]>63))
  return blocked(S::NeedsAttachment,0xb570);
 const bool banking=word(snapshot,0x28d4)!=0;
 if(banking&&(!scratch||!pending.reserve||!pending.publish))return blocked(S::NeedsPendingQueue,0x1105c);
 // Only expired BC0 retains the integrated W lane; other attachment paths
 // replace that quadword before it becomes observable.
 const bool retain_attachment_w=banking&&attached&&!(word(context,0x660)&8u)&&!(scalar(context,0xc0)>0)&&!context[0xbc];
 auto next=context;Rounding rounding;bool owner_emission=false,full_action=false,attachment_audio=false,attachment_emission=false,repair_particle=false;
 ArenaPlayerPeerResult peer_result;std::optional<ArenaRandom> peer_random;
 ArenaCounterEmissionRequest attachment_request;ArenaParticleEmission repair_request;
 std::array<ArenaParticleEmission,4> action_particles{};
 std::array<ArenaCounterEmissionRequest,4> action_requests{};unsigned action_count=0;
 ArenaCounterEmissionRequest request;
 auto staged=scratch?*scratch:ArenaPlayerControlScratch{};
 std::array<unsigned,4> proxy_links{0xc8,0xcc,0xca,0xce};
 try {
  const auto x=check(scalar(snapshot,base+0x10)),z=check(scalar(snapshot,base+0x14));
  if(std::abs(x)>1||std::abs(z)>1)return {};
  // The inherited aim proof excludes aligned/near-aligned Z. Reject that
  // domain here instead of introducing the preview's finite analytic limit.
  if(!automatic&&!context[0xbf]&&(x!=0||z!=0)&&std::abs(x)<mul(std::abs(z),.001f))return blocked(S::NeedsNumericRecovery,0x1a118);
  put(next,0x5a0,check(scalar(next,0x74))>0?6:0);
  std::copy_n(next.begin()+0xe0,16,next.begin()+0x480);
  store(next,0x20,vector(next,0xf0));next[0xbd]=0;
  if(peer_needed){
   peer_random=particles.random?*particles.random:ArenaRandom{};peer_result=prepare_arena_player_peer_contact(next,snapshot,*peer_random);
   if(!peer_result.advanced)return blocked(S::NeedsPeerContact,0x7e40);
   next[0xbd]=peer_result.overlapping;
   if(peer_result.outgoing_impulse&&(!scratch||!peer_accumulator.read_reserved||!peer_accumulator.write_conditional))return blocked(S::NeedsPeerContact,0x16f8);
   if(peer_result.particles&&!particles.publish)return blocked(S::NeedsContactEffects,0x7704);
  }
  V position;for(unsigned i=0;i<4;++i)position[i]=attached&&i==3?(retain_attachment_w?scalar(next,0x480+4*i):0.f):lane_check(scalar(next,0x480+4*i),i);
  auto velocity=vector(next,0x4a0);
  for(unsigned i=0;i<4;++i)position[i]=retain_attachment_w&&i==3?std::fma(velocity[i],elapsed,position[i]):lane_ma(velocity[i],elapsed,position[i],i);
  put(next,0x5a0,0); // 1053C
  ArenaGroundBoundsQuery bounds;bounds.position=position;bounds.velocity=velocity;
  bounds.accumulated_impulse=vector(next,0x4d0);
  bounds.max_x=float(word(snapshot,0x28d8));bounds.max_z=float(word(snapshot,0x28dc));bounds.max_y=2;
  bounds.radius=check(scalar(next,0x4e0));bounds.lateral_radius=check(scalar(next,0x4e4));
  bounds.restitution_percent=next[0x7d];bounds.movable=(word(next,0x60)&0x4000)!=0;
  bounds.floor_uses_lateral_radius=(word(next,0x60)&0x02000000u)!=0;
  auto contained=contain_arena_ground_unit(bounds);if(!contained)throw Numeric{};
  // 1053C clears5A0 before7DD0.7510 can call1E880 on a hard hit,
  // but7638..76F0 then supplies count0: no allocation, record or RNG draw.
  // Synchronous native storage also needs no corresponding DMA-tag wait.
  position=contained->position;velocity=contained->velocity;
  auto impulse=contained->accumulated_impulse;auto consumed=vector(next,0x340);
  for(unsigned i=0;i<4;++i)impulse[i]=lane_add(impulse[i],consumed[i],i);
  store(next,0x4d0,impulse);
  next[0xc7]=ma(impulse[2],impulse[2],ma(impulse[1],impulse[1],mul(impulse[0],impulse[0])))>3?1:0;
  V target{mul(x,banking?10.f:40.f),0,mul(z,banking?10.f:40.f),0};
  const V previous_horizontal{velocity[0],0,velocity[2],0};
  if(banking&&next[0xbe])for(auto& q:target)q=mul(q,bits(0x3f333333u));
  float response=banking?mul(elapsed,bits(0x40933333u)):1.f;
  if(automatic){
   // 11CA8..11DE4: approach the original byte-addressed grid target.
   const V goal{add(float(snapshot[base+0x2a]),.5f),1,add(float(snapshot[base+0x2b]),.5f),0};
   const auto delta=difference(goal,position);
   if(root(dot(delta,delta))>bits(0x3dcccccc)){
    for(unsigned i=0;i<4;++i)target[i]=mul(delta[i],4);
    const float length=root(dot(target,target));
    if(length>10){const float scale=divide(10,length);for(auto& component:target)component=mul(component,scale);}
    next[0xbd]=0;
    if(next[0xbe])for(auto& component:target)component=mul(component,bits(0x3f333333));
   }else for(unsigned i=0;i<4;++i)target[i]=lane_mul(velocity[i],bits(0x3e4ccccc),i);
   response=1;
  }
  for(unsigned i=0;i<4;++i)velocity[i]=lane_ma(lane_sub(target[i],velocity[i],i),response,velocity[i],i);
  velocity[1]=0;position[1]=1;
  if(attached){
   // Mode0 takes10648 directly: reset74 and load the source cached body quadword.
   // Its fourth lane is packed identity/type metadata, not a floating coordinate.
   if(banking&&!(word(next,0x660)&8u)){
    if(check(scalar(next,0xc0))>0){
     // 119BC..11A6C: positive countdown from the saved cell toward the body.
     const float timer=sub(check(scalar(next,0xc0)),elapsed);fp(next,0xc0,timer);
     const float blend=mul(sub(2,timer),.5f);
     const V center{add(float(half(next,0xd0)),.5f),0,add(float(half(next,0xd2)),.5f),0};
     for(unsigned i=0;i<3;++i)position[i]=ma(sub(check(scalar(next,0x600+4*i)),center[i]),blend,center[i]);
     position[1]=1;position[3]=std::fma(scalar(next,0x60c),blend,0.f);fp(next,0x38,1);
    }else if(next[0xbc]){
     // 11E08..11F6C: construction progress, health and Y interpolation.
     const float old=check(scalar(next,0x74));
     if(old==0){if(!audio)return blocked(S::NeedsAudio,0x11e38);attachment_audio=true;}
     float progress=ma(elapsed,.5f,old);fp(next,0x74,progress);
     next[0x33]=std::uint8_t((next[0x33]&~28u)|24u);next[0x6f]=0;
     if(progress>1){
      next[0xbc]=0;progress=1;fp(next,0x74,progress);
      if(check(scalar(next,0xd4))!=2){
       if(!scratch||!emitter)return blocked(S::NeedsActionEmission,0x1275c);
       auto descriptor=staged.attachment_effect;
       fp(descriptor,0,0);fp(descriptor,4,-5);fp(descriptor,8,0);descriptor[14]=0;descriptor[15]=108;
       std::copy_n(descriptor.begin(),16,descriptor.begin()+0x20);ph(descriptor,12,0);put(descriptor,0x10,16);
       ph(descriptor,0x30,half(next,12));descriptor[0x32]=descriptor[0x33]=1;put(descriptor,0x34,0);
       put(descriptor,0x38,word(next,0xd4));fp(descriptor,0x3c,2);put(descriptor,0x40,0);
       put(descriptor,0x44,0x3e20d97b);put(descriptor,0x50,owner);descriptor[0x54]=0;
       attachment_request={descriptor,1,0};attachment_emission=true;
      }
     }
     fp(next,0x78,progress);fp(next,0x38,mul(progress,check(scalar(next,0x64))));velocity={};
     for(unsigned i=0;i<4;++i){
      const float fetched=i==3?scalar(next,0x60c):check(scalar(next,0x600+4*i));
      const float goal=i==1?1.f:fetched;
      position[i]=i==3?std::fma(goal-fetched,progress,fetched):ma(sub(goal,fetched),progress,fetched);
     }
     if(!std::isfinite(position[3]))throw Numeric{};
    }else{
     // 11F70..12194: health regeneration near a same-group player cell.
     const float health=check(scalar(next,0x38)),maximum=check(scalar(next,0x64));
     if(maximum>health){
      const unsigned players=word(snapshot,0x2820);if(players>4)return {};
      bool eligible=false;
      for(unsigned p=0;p<players;++p){
       const auto dx=std::bit_cast<std::int16_t>(std::uint16_t(half(next,0x552)-std::uint16_t(word(snapshot,0x2660+4*p))));
       const auto dz=std::bit_cast<std::int16_t>(std::uint16_t(half(next,0x562)-std::uint16_t(word(snapshot,0x2670+4*p))));
       eligible|=half(snapshot,0x2720+2*p)==half(snapshot,0x2720+2*owner)&&dx>=-1&&dx<=1&&dz>=-1&&dz<=1;
      }
      if(eligible){
       if(!particles.random||!particles.publish)return blocked(S::NeedsContactEffects,0x12188);
       const float updated=ma(maximum,mul(elapsed,bits(0x3dcccccc)),health);fp(next,0x38,maximum>updated?updated:maximum);
       repair_particle=true;repair_request.position=position;repair_request.radius_min=repair_request.radius_max=1;
       repair_request.speed=-5;repair_request.lifetime_min=repair_request.lifetime_max=bits(0x3e5d2f18);repair_request.count=1;
       fp(repair_request.configuration,0,bits(0x3dcccccc));repair_request.configuration[8]=37;repair_request.configuration[9]=1;
      }
     }
    }
   }else{
    fp(next,0x74,0);for(unsigned i=0;i<3;++i)position[i]=check(scalar(next,0x600+4*i));
    position[3]=scalar(next,0x60c);
   }
  }
  store(next,0x480,position);store(next,0x4a0,velocity);
  // 11640 sentinel branch: reset action progress and staged collision mask.
  if(!attached){put(next,0x5a0,0);fp(next,0x78,std::bit_cast<float>(0x3c23d70au));next[0x33]&=0xe3;}
  ArenaCraftFacingState facing;facing.yaw=check(scalar(next,0xdc));facing.pending_orientation=vector(next,0xf0);
  const auto visible=facing.pending_orientation;
  if(next[0xbf]){
   V direction{check(scalar(next,0x200)),check(scalar(next,0x204)),check(scalar(next,0x208)),0};
   for(unsigned i=0;i<3;++i)direction[i]=sub(direction[i],position[i]);
   const auto factor=inverse(dot(direction,direction));
   for(auto& component:direction)component=mul(component,factor);
   if(std::abs(direction[0])<mul(std::abs(direction[2]),.001f))return blocked(S::NeedsNumericRecovery,0x1a118);
   ArenaWeaponAimQuery aim;aim.direction=direction;aim.yaw=facing.yaw;
   aim.yaw_step=mul(mul(elapsed,std::bit_cast<float>(0x41490fdau)),std::bit_cast<float>(0x3f666666u));
   const auto result=prepare_arena_weapon_aim(aim);if(!result)throw Numeric{};
   facing.yaw=result->yaw;facing.pending_orientation=result->source_orientation;
  }else if(automatic){
   // 11558..11614 consumes the approach vector directly (including Y).
   const float length=root(dot(target,target));
   if(length>bits(0x3c23d70a)){
    const float estimate=detail::reciprocal_estimate(length);
    const float residual=nm(length,estimate,bits(0x3f800001));
    ArenaWeaponAimQuery aim;
    for(unsigned i=0;i<4;++i){const float product=mul(target[i],estimate);aim.direction[i]=ma(residual,product,product);}
    if(std::abs(aim.direction[0])<mul(std::abs(aim.direction[2]),.001f))return blocked(S::NeedsNumericRecovery,0x1a118);
    aim.yaw=facing.yaw;aim.yaw_step=mul(elapsed,bits(0x41490fda));
    const auto result=prepare_arena_weapon_aim(aim);if(!result)throw Numeric{};
    facing.yaw=result->yaw;facing.pending_orientation=result->source_orientation;
   }
  }else advance_arena_craft_facing(facing,x,z,elapsed,banking,next[0xbe]!=0);
  fp(next,0xdc,facing.yaw);store(next,0x20,visible);store(next,0xf0,facing.pending_orientation);
  const V gripper_left{bits(0xbea66666),bits(0xbdd4fdf2),bits(0xbf15c28e),0},
          gripper_right{bits(0x3ea66666),bits(0xbdd4fdf2),bits(0xbf15c28e),0};
  const auto progress=check(scalar(next,0x78));
  V left_point{},right_point{},left_orientation=facing.pending_orientation,right_orientation=facing.pending_orientation;
  if(banking){V old_position{};for(unsigned i=0;i<3;++i)old_position[i]=check(scalar(next,0xe0+4*i));
   left_point=point(old_position,visible,gripper_left,progress);
   right_point=point(old_position,visible,gripper_right,progress);}
  const auto action=check(scalar(snapshot,base+0x34));
  if(action>0&&!next[0xbc]){
   if(next[0xbb]){
    if(!scratch||!emitter)return blocked(S::NeedsActionEmission,0x12b5c);
    auto descriptor=scratch->action_effect;
    for(bool second_target:{false,true})for(unsigned slot=0;slot<2;++slot){
     auto visual=action_visual_request(descriptor,next,slot,second_target);
     descriptor=visual.descriptor;action_requests[action_count++]=visual;
    }
    next[0xbb]=0;
   }
   bool eligible=false;
   if(half(next,0x1e)!=65535){
    V candidate{check(scalar(next,0x200)),check(scalar(next,0x204)),check(scalar(next,0x208)),0};
    const auto separation=difference(candidate,position);
    if(next[0x20f]<=60&&root(dot(separation,separation))<3){
     // 108F0..10910: segment from half to full current forward vector.
     // The separately computed old gripper points only feed dead mode0 quaternions.
     const auto forward=rotate_point(vector(next,0xf0),V{0,0,-1,0});
     V middle{},endpoint{};for(unsigned i=0;i<3;++i){middle[i]=ma(forward[i],.5f,position[i]);endpoint[i]=add(position[i],forward[i]);}
     eligible=true;next[0xbf]=1;
     if(.5f>segment_distance(candidate,middle,endpoint))next[0xbe]=1;
    }
   }
   if(!eligible){next[0xbe]=next[0xbf]=0;ph(next,0x1e,word(next,0x378));}
   if(banking){
    V candidate=left_point;
    if(eligible)for(unsigned i=0;i<3;++i)candidate[i]=check(scalar(next,0x200+4*i));
    auto l=normalize(difference(candidate,left_point)),r=normalize(difference(candidate,right_point));
    for(unsigned i=0;i<4;++i){l[i]=-l[i];r[i]=-r[i];}
    left_orientation=direction_orientation(l);right_orientation=direction_orientation(r);
   }
   // 10A28's two quaternion results are stack-only and dead in mode0.
  }else{
   // 113E0: button release rearms the edge and clears target state.
   ph(next,0x1e,65535);next[0xbe]=next[0xbf]=0;next[0xbb]=1;
  }
  next[0x590]=std::uint8_t((!(check(scalar(next,0x38))>0))&(next[0xbc]^1u));
  if(next[0x590])return blocked(S::NeedsDeath,0x12288);
  if(banking){
   // 10B60..10EEC: body-relative arm roots, angular banking and smoothed
   // arm vectors. Source20 holds the new facing until the final delayed exchange.
   const auto q=facing.pending_orientation;
   const V left_root{bits(0xbed2fec3),bits(0xbd2a64c1),bits(0x3e5a5118),0},
           right_root{bits(0x3ed2fec3),bits(0xbd2a64c1),bits(0x3e5a5118),0};
   const auto left_rotated_root=rotate_point(q,left_root),right_rotated_root=rotate_point(q,right_root);
   V left_base{},right_base{};
   for(unsigned i=0;i<4;++i){
    // W can contain packed fetched-body bits; retain it without filtering.
    left_base[i]=std::fma(left_rotated_root[i],check(scalar(next,0x78)),position[i]);
    right_base[i]=std::fma(right_rotated_root[i],check(scalar(next,0x78)),position[i]);
   }
   const float factor=divide(bits(0x3c888888),check(scalar(snapshot,0x27b0)));
   V acceleration{};for(unsigned i=0;i<4;++i)acceleration[i]=lane_mul(lane_sub(velocity[i],previous_horizontal[i],i),factor,i);
   float angle=sub(facing.yaw,check(scalar(context,0xdc)));
   if(angle>bits(0x40490fda))angle=sub(angle,bits(0x40c90fda));
   else if(bits(0xc0490fda)>angle)angle=add(angle,bits(0x40c90fda));
   const float turn=mul(factor,mul(angle,10));
   V arm_left{bits(0xbec28f5c),1,bits(0x3d4ccccc),0},arm_right{bits(0x3ec28f5c),1,bits(0x3d4ccccc),0};
   arm_left[2]=add(arm_left[2],turn);arm_right[2]=sub(arm_right[2],turn);
   const auto left_rotated=rotate_point(q,arm_left),right_rotated=rotate_point(q,arm_right);
   V left_goal{},right_goal{};
   for(unsigned i=0;i<4;++i){left_goal[i]=lane_add(lane_mul(acceleration[i],3,i),left_rotated[i],i);
    right_goal[i]=lane_add(lane_mul(acceleration[i],3,i),right_rotated[i],i);}
   const float left_inverse=inverse(dot(left_goal,left_goal)),right_inverse=inverse(dot(right_goal,right_goal));
   auto left_arm=vector(next,0x90),right_arm=vector(next,0xa0);
   for(unsigned i=0;i<4;++i){left_arm[i]=lane_ma(lane_ma(left_goal[i],left_inverse,-left_arm[i],i),bits(0x3e4ccccc),left_arm[i],i);
    right_arm[i]=lane_ma(lane_ma(right_goal[i],right_inverse,-right_arm[i],i),bits(0x3e4ccccc),right_arm[i],i);}
   left_arm=normalize(left_arm);right_arm=normalize(right_arm);
   store(next,0x90,left_arm);store(next,0xa0,right_arm);
   for(auto& component:left_arm)component=-component;
   for(auto& component:right_arm)component=-component;
   const auto left_bank=direction_orientation(left_arm),right_bank=direction_orientation(right_arm);
   proxy_descriptor(staged.interaction_proxies[0],next,left_point,left_orientation,40,half(next,0xc8),scalar(next,0x78));
   proxy_descriptor(staged.interaction_proxies[1],next,left_base,left_bank,1,half(next,0xcc),scalar(next,0x78));
   proxy_descriptor(staged.interaction_proxies[2],next,right_point,right_orientation,40,half(next,0xca),scalar(next,0x78));
   proxy_descriptor(staged.interaction_proxies[3],next,right_base,right_bank,1,half(next,0xce),scalar(next,0x78));
   // 11214 ->11A80..11CA4: four ordered arm-exhaust particle calls.
   if(scalar(next,0x78)==1){
    if(!particles.random||!particles.publish)return blocked(S::NeedsContactEffects,0x11a80);
    full_action=true;
    for(unsigned n=0;n<4;++n){
     auto& q=action_particles[n];const auto arm=vector(next,n%2?0xa0:0x90);const auto& base=n%2?right_base:left_base;
     for(unsigned i=0;i<4;++i){
      q.position[i]=i==3?std::fma(-arm[i],bits(0x3dcccccc),base[i]):nm(arm[i],bits(0x3dcccccc),base[i]);
      if(!std::isfinite(q.position[i]))throw Numeric{};
      q.velocity[i]=lane_mul(arm[i],-4,i);
     }
     q.radius_min=0;q.radius_max=bits(0x3dcccccc);q.count=1;q.speed=bits(0x3cf5c28f);
     q.lifetime_min=bits(0x3e4ccccc);q.lifetime_max=bits(0x3e999999);
     fp(q.configuration,0,bits(0x3d0b4395));fp(q.configuration,4,-2);
     q.configuration[9]=1;q.configuration[10]=0;q.configuration[11]=1;q.configuration[12]=0;
    }
   }
  }
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
  std::copy_n(context.begin()+0xe0,16,next.begin()+0x480);store(next,0xe0,position);
 }catch(const Numeric&){return blocked(S::NeedsNumericRecovery,0x10290);}
 catch(const std::invalid_argument&){return blocked(S::NeedsNumericRecovery,0x1a118);}
 // All rejecting computation precedes external mutations. Queue callbacks may
 // only observe their supplied descriptor, so deferred pure work is unobservable.
 if(pending_needed){
  pending_descriptor(staged.pending_entity,context);
  const auto index=pending.reserve(staged.pending_entity);
  if(index!=-1){
   if(index<0||index>65535)throw std::logic_error("invalid pending entity identity");
   ph(staged.pending_entity,12,unsigned(index));put(next,0x60,word(next,0x60)&~0x8000u);
   pending.publish(staged.pending_entity);
  }
 }
 if(peer_random&&particles.random)*particles.random=*peer_random;
 if(peer_result.outgoing_impulse){
  const auto ea=word(snapshot,0x27f8)+std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(peer_result.peer_index)))*128u;
  if(submit_arena_peer_impulse(staged.peer_accumulator,ea,peer_result.impulse,peer_accumulator)!=ArenaAccumulatorStatus::Cached)
   throw std::runtime_error("original peer accumulator transport failed");
 }
 if(peer_result.particles)particles.publish(*peer_result.particles,*particles.random);
 if(attachment_audio)audio(ArenaAudioEvent{8,0,0});
 if(attachment_emission){emitter(attachment_request);staged.attachment_effect=attachment_request.descriptor;}
 if(repair_particle)particles.publish(repair_request,*particles.random);
 for(unsigned i=0;i<action_count;++i){emitter(action_requests[i]);staged.action_effect=action_requests[i].descriptor;}
 if(banking)for(unsigned i=0;i<4;++i)publish_proxy(next,proxy_links[i],staged.interaction_proxies[i],pending,word(snapshot,0x27c0));
 if(full_action)for(auto& q:action_particles){
  q.configuration[8]=std::uint8_t(85+(particles.random->next()&3u));
  particles.publish(q,*particles.random);
 }
 if(owner_emission){emitter(request);staged.owner_effect=request.descriptor;}
 if(scratch)*scratch=staged;
 context=next;return {S::Advanced,0};
}
}
