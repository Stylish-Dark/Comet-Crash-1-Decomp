#include "comet/arena_ground_carry.hpp"
#include "comet/arena_ground_orientation.hpp"
#include "comet/arena_ground_contact.hpp"
#include "comet/arena_ground_steering.hpp"
#include "comet/arena_ground_height.hpp"
#include "comet/arena_player_peer.hpp"
#include "arena_spu_math.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {namespace {
using V=std::array<float,4>;
unsigned word(const auto& b,unsigned o){return unsigned(b[o])<<24|unsigned(b[o+1])<<16|unsigned(b[o+2])<<8|b[o+3];}
unsigned half(const auto& b,unsigned o){return unsigned(b[o])<<8|b[o+1];}
float bits(unsigned v){return std::bit_cast<float>(v);}
float scalar(const auto& b,unsigned o){return bits(word(b,o));}
void put(auto& b,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(24-8*i));}
void fp(auto& b,unsigned o,float v){put(b,o,std::bit_cast<unsigned>(v));}
V vector(const auto& b,unsigned o){V v;for(unsigned i=0;i<4;++i)v[i]=scalar(b,o+4*i);return v;}
void store(auto& b,unsigned o,const V& v){for(unsigned i=0;i<4;++i)fp(b,o+4*i,v[i]);}
bool number(float f){return std::isfinite(f)&&std::fpclassify(f)!=FP_SUBNORMAL&&std::abs(f)<=1e6f;}
bool valid(const V& v,bool packed=false){for(unsigned i=0;i<4;++i)if(packed&&i==3?!std::isfinite(v[i]):!number(v[i]))return false;return true;}
float mul(float a,float b){volatile float v=a*b;return v;}
float add(float a,float b){volatile float v=a+b;return v;}
float sub(float a,float b){volatile float v=a-b;return v;}
float ma(float a,float b,float c){return std::fma(a,b,c);}
float nm(float a,float b,float c){return std::fma(-a,b,c);}
float dot(const V& v){return ma(v[2],v[2],ma(v[1],v[1],mul(v[0],v[0])));}
float length(float s){const float e=detail::inverse_root_estimate(s),r=mul(e,s);return ma(nm(e,r,bits(0x3f800001)),mul(r,.5f),r);}
float divide(float a,float b){const float e=detail::reciprocal_estimate(b),v=mul(a,e);return ma(nm(v,b,a),e,v);}
V rotate(const V& q,const V& p){
 const float projection=add(ma(p[1],q[1],mul(q[0],p[0])),mul(q[2],p[2]));V first{},out{};
 for(unsigned i=0;i<4;++i){const auto j=i==3?3:(i+1)%3,k=i==3?3:(i+2)%3;first[i]=nm(q[k],p[j],ma(q[j],p[k],mul(q[3],p[i])));}
 for(unsigned i=0;i<4;++i){const auto j=i==3?3:(i+1)%3,k=i==3?3:(i+2)%3;out[i]=nm(q[k],first[j],ma(q[j],first[k],ma(q[3],first[i],mul(projection,q[i]))));}return out;
}
struct Rounding{int old=std::fegetround();Rounding(){if(old<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("carry rounding unavailable");}~Rounding(){std::fesetround(old);}};
using S=ArenaGroundCarryStatus;
bool carried(const auto& c){return c[15]>=40&&c[15]<=48&&word(c,0x500)==c[15]&&word(c,0x5d0)==0x80&&half(c,0xb4)!=65535;}
}
bool fetch_arena_ground_carry_references(ArenaGroundCarryContext& context,
 std::span<const ArenaEntitySlot> bank,std::span<const ArenaAccumulatorRecord> accumulators,std::uint32_t tag){
 if(context[15]<40||context[15]>48||half(context,12)>=accumulators.size())return false;
 const std::array<unsigned,4> indices{half(context,0x1c),half(context,0x1e),half(context,0xb4),half(context,0xb6)};
 for(auto index:indices)if(index!=65535&&(index>=32768||index>=bank.size()))return false;
 auto next=context;std::copy(accumulators[half(context,12)].begin(),accumulators[half(context,12)].end(),next.begin()+0x300);
 for(unsigned n=0;n<4;++n)if(indices[n]!=65535){const unsigned offset=n<2?0x100+256*n:0xa00+256*(n-2);std::copy(bank[indices[n]].begin(),bank[indices[n]].end(),next.begin()+offset);}
 put(next,0x59c,tag);context=next;return true;
}
S prepare_arena_ground_carry_position(ArenaGroundCarryContext& context,
 std::span<const std::uint8_t,0x2900> snapshot,float elapsed){
 if(!carried(context))return S::NeedsParent;
 if(!number(elapsed)||elapsed<0||elapsed>.25f)return S::Invalid;
 if(context[0xa0f]==63)return S::NeedsCaller; //14158..14188 ordinary motion
 const auto q=vector(context,0xa20),parent=vector(context,0xa00),position=vector(context,0x480);
 if(!valid(q)||!valid(parent,true)||!valid(position,true))return S::NeedsNumericRecovery;
 auto next=context;Rounding rounding;
 const unsigned owner=context[0xa33]>>5;
 if(word(context,0x530)==half(snapshot,0x2720+2*owner))next[0x7e]=context[0xa7e];
 const auto index=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(half(context,0xb2))));
 const float column=ma(float(index%5),bits(0x3e6147ae),-bits(0x3ee147ae));
 const float row=ma(float(index/5),bits(0x3e6147ae),bits(0x3e999999));
 const auto x=rotate(q,{1,0,0,0}),z=rotate(q,{0,0,1,0});V delta{};
 for(unsigned i=0;i<4;++i){const float center=add(parent[i],i==1?bits(0x3eb33333):0.f);delta[i]=sub(ma(z[i],row,ma(x[i],column,center)),position[i]);}
 const float squared=dot(delta);if(!number(squared))return S::NeedsNumericRecovery;
 // Source estimates zero lengths outside the supported IEEE proof domain.
 if(squared==0)return S::NeedsNumericRecovery;
 const float distance=length(squared),limit=mul(elapsed,1.5f);
 if(!number(distance)||distance<=0)return S::NeedsNumericRecovery;
 if(distance>limit){const float scale=divide(limit,distance);for(auto& f:delta)f=mul(f,scale);}
 V moved{};for(unsigned i=0;i<4;++i)moved[i]=add(position[i],delta[i]);
 if(!valid(moved,true))return S::NeedsNumericRecovery;
 store(next,0x480,moved);context=next;return S::Advanced;
}
S advance_prepared_arena_linked_ground_motion(ArenaGroundCarryContext& context,
 std::span<const std::uint8_t,0x2900> snapshot,float elapsed,ArenaRandom* random,ArenaPlayerPeerResult* deferred_peer){
 if(!carried(context))return S::NeedsParent;
 const unsigned width=word(snapshot,0x28d8),height=word(snapshot,0x28dc),owner=half(context,0x542);
 if(!width||width>24||height!=width||owner>=4||!number(elapsed)||elapsed<0||elapsed>.25f)return S::Invalid;
 //18030 passes r4=1 exactly for stage5. Other stages reach14820's separate
 // approach/deployment branch, not the route branch recovered here.
 if((context[0x33]&28u)!=20)return S::NeedsCaller;
 const unsigned initial_target=context[0x7e];
 if(initial_target!=255&&initial_target>=4)return S::Invalid;
 const bool target_valid=initial_target!=255&&word(snapshot,0x2770+4*initial_target)!=0xffffffffu;
 const unsigned players=word(snapshot,0x2820);if(players>4)return S::Invalid;
 if(context[0xa0f]==63)return S::NeedsCaller;
 const bool live_peer=context[0x10f]<62&&half(context,0x1c)!=65535&&half(context,0x1c)!=half(context,0x6c);
 if(live_peer&&!random)return S::NeedsPeerContact;
 auto rng=random?*random:ArenaRandom{};
 auto next=context;Rounding rounding;next[0x598]=target_valid;
 if(!target_valid){
  //12BE0 prefers owner+257E when its base is live.3068 otherwise chooses
  // from2CE0's ascending enemy list using one original shuffled RNG draw.
  const unsigned forced=snapshot[0x257e + 64*owner];unsigned target=255;
  if(forced!=255){if(forced>=4)return S::Invalid;if(word(snapshot,0x2770+4*forced)!=0xffffffffu)target=forced;}
  if(target==255){
   std::array<unsigned,4> candidates{};unsigned count=0;
   for(unsigned p=0;p<players;++p)if(snapshot[0x2578+64*p]!=snapshot[0x2578+64*owner]&&word(snapshot,0x2770+4*p)!=0xffffffffu)candidates[count++]=p;
   if(count){if(!random)return S::NeedsCaller;target=candidates[rng.next()%count];}
  }
  next[0x7e]=std::uint8_t(target);next[0x598]=target!=255;
 }
 const auto formed=prepare_arena_ground_carry_position(next,snapshot,elapsed);
 if(formed!=S::Advanced)return formed;
 const unsigned target=next[0x7e];
 if(next[0x598]&&(target>=4||target==owner))return S::NeedsCaller;
 //14188 calls7E40 before integrating. A stale fetched record clears the
 // reference even when the previous link was alreadyFFFF.
 ArenaEntityWorkContext prefix;std::copy_n(next.begin(),prefix.size(),prefix.begin());
 const auto peer=prepare_arena_ground_peer_contact(prefix,snapshot,rng);
 if(!peer.advanced)return S::NeedsPeerContact;
 if((peer.outgoing_impulse||peer.particles)&&!deferred_peer)return S::NeedsContactEffects;
 std::copy(prefix.begin(),prefix.end(),next.begin());
 auto position=vector(next,0x480);const auto velocity=vector(next,0x4a0);
 if(!valid(velocity,true)||!valid(vector(next,0x4d0),true))return S::NeedsNumericRecovery;
 for(unsigned i=0;i<4;++i)position[i]=ma(velocity[i],elapsed,position[i]);
 if(!valid(position,true))return S::NeedsNumericRecovery;
 // Linked B4 bypasses local terrain20 and always invokes7DD0 world bounds.
 ArenaGroundBoundsQuery q;q.position=position;q.velocity=velocity;q.accumulated_impulse=vector(next,0x4d0);
 q.max_x=float(width);q.max_z=float(height);q.radius=scalar(next,0x4e0);q.lateral_radius=scalar(next,0x4e4);
 q.restitution_percent=next[0x7d];q.movable=(word(next,0x60)&0x4000u)!=0;
 q.floor_uses_lateral_radius=(word(next,0x60)&0x02000000u)!=0;
 const auto bounded=contain_arena_ground_unit(q);if(!bounded)return S::NeedsNumericRecovery;
 //7510 can publish particles on an energetic world hit. Never suppress them.
 if(bounded->energetic_impact&&word(next,0x5a0))return S::NeedsContactEffects;
 store(next,0x480,bounded->position);store(next,0x4a0,bounded->velocity);store(next,0x4d0,bounded->accumulated_impulse);
 next[0xba]=bounded->contacts!=0;
 const unsigned x=unsigned(std::clamp(int(bounded->position[0]),0,int(width)-1)),z=unsigned(std::clamp(int(bounded->position[2]),0,int(height)-1));
 auto ph=[&](unsigned o,unsigned v){next[o]=std::uint8_t(v>>8);next[o+1]=std::uint8_t(v);};
 ph(0x552,x);ph(0x562,z);
 if(half(next,0xcc)!=x||half(next,0xce)!=z){ph(0xc8,half(next,0xcc));ph(0xca,half(next,0xce));ph(0xcc,x);ph(0xce,z);}
 fp(next,0x574,32);
 ArenaRoutingGrid grid;
 for(unsigned n=0;n<576;++n){grid.cells[n].visibility=snapshot[16*n+3];std::copy_n(snapshot.begin()+16*n+4,12,grid.cells[n].routes.begin());}
 ArenaGroundSteeringQuery steering;steering.position=bounded->position;steering.orientation=vector(next,0x20);
 steering.owner=std::uint8_t(owner);steering.target=std::uint8_t(target);steering.extent=std::uint8_t(width);
 steering.visibility_bit=std::uint8_t(word(next,0x530));steering.target_available=next[0x598]!=0;
 const auto tx=steering.target_available?word(snapshot,0x2660+4*target):0u,tz=steering.target_available?word(snapshot,0x2670+4*target):0u;
 if(tx>=width||tz>=height||word(next,0x530)>=32)return S::Invalid;
 steering.target_base={std::uint8_t(tx),std::uint8_t(tz)};steering.direct_route=next[0x595]!=0;
 steering.suppressed_cell={std::bit_cast<std::int16_t>(std::uint16_t(half(next,0xc8))),std::bit_cast<std::int16_t>(std::uint16_t(half(next,0xca)))};
 const auto direction=steer_arena_ground_unit(steering,grid);if(!direction)return S::NeedsNumericRecovery;
 if(steering.target_available)next[0x596]=direction->blocked;
 ArenaGroundHeightQuery h;h.position=bounded->position;h.velocity=bounded->velocity;h.direction=direction->direction;h.enabled=steering.target_available&&next[0x595]!=0;
 const auto elevated=steer_arena_ground_height(h);if(!elevated)return S::NeedsNumericRecovery;
 store(next,0x5c0,*elevated);context=next;if(random)*random=rng;if(deferred_peer)*deferred_peer=peer;return S::Advanced;
}
S advance_arena_ground_carry_dynamics(ArenaGroundCarryContext& context,
 std::span<const std::uint8_t,0x2900> snapshot,float elapsed){
 if(!carried(context))return S::NeedsParent;
 if(!number(elapsed)||elapsed<0||elapsed>.25f)return S::Invalid;
 if(!number(scalar(context,0x8c)))return S::NeedsNumericRecovery;
 if(context[0x596]||scalar(context,0x8c)>0)return S::NeedsCaller;
 if((word(context,0x510)==6||context[0x595]!=1)&&
    (context[15]==45||context[15]==47||context[15]==48))return S::NeedsCaller;
 if(!number(scalar(context,0x18)))return S::NeedsNumericRecovery;
 const auto flags=word(context,0x60)&~0x800u;
 //15F94 onward command/effect paths must not be silently omitted.
 if(word(context,0x370)||word(context,0x360)==2||word(context,0x360)==3||
    ((flags&0x1000u)&&scalar(context,0x570)<bits(0x3ecccccc)&&(word(context,0x160)&0x2000u)&&!(flags&0x200000u)))return S::NeedsCommands;
 if(!number(scalar(context,0xc4)))return S::NeedsNumericRecovery;
 auto next=context;Rounding rounding;
 const float timer=sub(scalar(context,0xc4),elapsed);
 if(timer<0&&word(snapshot,0x28e8)!=0xffffffffu)return S::NeedsCommands;
 put(next,0xc0,0); //15978..15994
 put(next,0x60,flags);fp(next,0x18,ma(sub(1.f,scalar(next,0x18)),bits(0x3ecccccc),scalar(next,0x18)));
 put(next,0x10,124);next[0x33]=std::uint8_t((next[0x33]&~28u)|(next[0xa33]&28u));
 if(next[0xa0f]>=62){
  put(next,0x60,flags&~0x1000u);next[0xb4]=next[0xb5]=next[0xb6]=next[0xb7]=255;
  next[0xb8]=1;put(next,0x8c,0x40000000);next[0x33]=std::uint8_t((next[0x33]&~28u)|20u);
 }else{
  auto velocity=vector(next,0x4a0);const auto position=vector(next,0x480);
  if(!valid(velocity,true)||!valid(position,true)||!number(scalar(next,0xa04))||!number(scalar(next,4)))return S::NeedsNumericRecovery;
  V acceleration{};const V mask{1,0,1,0};for(unsigned i=0;i<4;++i)acceleration[i]=mul(mul(velocity[i],mask[i]),-2.f);
  if(scalar(next,0xa04)>scalar(next,4))acceleration[1]=add(acceleration[1],ma(velocity[1],-1.f,sub(scalar(next,0xa04),position[1])));
  for(unsigned i=0;i<4;++i)velocity[i]=ma(acceleration[i],elapsed,velocity[i]);
  const float squared=dot(velocity);if(!number(squared))return S::NeedsNumericRecovery;
  if(squared>0){const float speed=length(squared);if(!number(speed))return S::NeedsNumericRecovery;
   if(speed>.5f){const float e=detail::inverse_root_estimate(squared),r=mul(e,squared);const float refined=ma(nm(e,r,bits(0x3f800001)),mul(e,.5f),e);for(auto& f:velocity)f=mul(mul(f,refined),.5f);}}
  const auto orientation=prepare_arena_ground_carried_orientation(vector(next,0x20),vector(next,0xa20),elapsed,scalar(next,0xe8),scalar(next,0x90));
  if(!orientation||!valid(velocity,true))return S::NeedsNumericRecovery;
  store(next,0x4a0,velocity);store(next,0x20,*orientation);
 }
 const auto tail_flags=word(next,0x60);
 const bool timeout=word(next,0xbc)==1&&(tail_flags&0x200000u);
 put(next,0xbc,timeout?0:3);
 if(timeout){const auto slots=std::uint16_t(half(next,0xb0)+1);next[0xb0]=std::uint8_t(slots>>8);next[0xb1]=std::uint8_t(slots);
  put(next,0x60,(tail_flags&~0x200000u)|(next[15]==47?0x1000u:0u));}
 fp(next,0xc4,timer);context=next;return S::Advanced;
}
S commit_prepared_arena_ground_carry(ArenaEntityStorage& storage,std::uint16_t index,
 ArenaGroundCarryContext& context,std::span<std::uint8_t,128> statistics,
 std::span<const std::uint8_t,0x2900> snapshot,float elapsed,std::uint32_t frame,
 ArenaGridPosition original_job_cell,
 const ArenaHealthDisplayTransport& display){
 if(storage.read_bank>1||index>=storage.banks[storage.read_bank].size()||half(context,12)!=index||context[15]<40||context[15]>48)return S::Invalid;
 auto next=context;const auto advanced=advance_arena_ground_carry_dynamics(next,snapshot,elapsed);if(advanced!=S::Advanced)return advanced;
 const auto result=finalize_prepared_arena_ground_carry(storage,index,next,statistics,snapshot,frame,original_job_cell,display);
 if(result==S::Advanced)context=next;return result;
}
S finalize_prepared_arena_ground_carry(ArenaEntityStorage& storage,std::uint16_t index,
 ArenaGroundCarryContext& context,std::span<std::uint8_t,128> statistics,
 std::span<const std::uint8_t,0x2900> snapshot,std::uint32_t frame,
 ArenaGridPosition original_job_cell,const ArenaHealthDisplayTransport& display){
 if(storage.read_bank>1||index>=storage.banks[storage.read_bank].size()||half(context,12)!=index||context[15]<40||context[15]>48)return S::Invalid;
 auto next=context;
 //6F40 alive/no-base path: team byte2520+owner*64+58, four base
 // tile coordinates2660/2670, entity indices2770 and enable bytes28E0.
 if(!(scalar(next,0x38)>0))return S::NeedsFinalization;
 const unsigned owner=half(next,0x542);if(owner>=4)return S::Invalid;
 Rounding rounding;
 const auto position=vector(next,0x480);if(!valid(position,true))return S::NeedsNumericRecovery;
 const auto team=snapshot[0x2578+64*owner];
 for(unsigned n=0;n<4;++n){
  if(snapshot[0x2578+64*n]==team||word(snapshot,0x2770+4*n)==0xffffffffu||(snapshot[0x28e0+n]&1u))continue;
  const auto x=std::bit_cast<std::int32_t>(word(snapshot,0x2660+4*n));
  const auto z=std::bit_cast<std::int32_t>(word(snapshot,0x2670+4*n));
  if(x < -1000000 || x > 1000000 || z < -1000000 || z > 1000000)return S::NeedsNumericRecovery;
  V delta{sub(add(float(x),.5f),position[0]),-position[1],sub(add(float(z),.5f),position[2]),0};
  const auto squared=dot(delta);if(!number(squared)||squared==0)return S::NeedsNumericRecovery;
  if(length(squared)<.5f&&!(next[0x591]&1u))return S::NeedsFinalization; //71BC base event/audio
 }
 if(original_job_cell.x>=storage.width||original_job_cell.z>=storage.height)return S::Invalid;
 if(!select_arena_entity_peer(next,arena_spatial_neighborhood(storage,original_job_cell)))return S::NeedsNumericRecovery;
 next[0x590]=0; //6FFC..701C alive predicate
 ArenaEntitySlot body;std::copy_n(next.begin(),256,body.begin());
 ArenaEntityFinalizationContext c;c.position_480=vector(next,0x480);c.velocity_4a0=vector(next,0x4a0);c.previous_scalar_38_4e8=scalar(next,0x4e8);
 c.statistics_owner_542=std::uint16_t(half(next,0x542));c.changed_59a=next[0x59a];c.auxiliary_596=next[0x596];c.health_display={word(next,0x500),word(next,0x520),scalar(next,0x4e4)};
 const auto result=finalize_arena_entity_update(storage,index,body,statistics,c,frame,display);
 if(result==ArenaEntityFinalization::CellFull)return S::CellFull;
 if(result!=ArenaEntityFinalization::Committed)return S::NeedsFinalization;
 std::copy_n(storage.banks[storage.read_bank][index].begin(),256,next.begin());context=next;return S::Advanced;
}
S complete_arena_ground_carry_command(ArenaGroundCarryContext& context,
 std::span<const std::uint8_t,0x2900> snapshot,float elapsed,
 ArenaAccumulatorRecord& scratch,const ArenaAccumulatorTransport& transport,
 ArenaCommandClaimResult* claim,ArenaGridPosition original_job_cell,const ArenaAudioEventSink& audio){
 if(context[15]<40||context[15]>48||word(context,0x500)!=context[15]||word(context,0x5d0)!=0x80)return S::Invalid;
 if(!number(elapsed)||elapsed<0||elapsed>.25f||!number(scalar(context,0xc4)))return S::Invalid;
 const auto flags=word(context,0x60);
 if(word(context,0x370))return S::NeedsCommands;
 const auto opcode=word(context,0x360);
 if(opcode==3&&!audio)return S::NeedsAudio;
 Rounding rounding;const auto timer=sub(scalar(context,0xc4),elapsed);
 if(timer<0&&word(snapshot,0x28e8)!=0xffffffffu)return S::NeedsCommands;
 //16710 skips the BC decrement and goes straight16060 after the claim.
 auto next=context;auto buffer=scratch;
 if(opcode==2){
 put(buffer,4,0);buffer[0x64]=context[12];buffer[0x65]=context[13];
 if(flags&0x2000u){
  put(next,0x360,3);std::array<std::uint8_t,16> payload;
  std::copy_n(next.begin()+0x360,16,payload.begin());
  const auto result=claim_arena_entity_command(buffer,word(snapshot,0x27f8),
      std::bit_cast<std::int16_t>(std::uint16_t(half(next,0x364))),payload,transport);
  if(claim)*claim=result;
  if(result.status==ArenaCommandClaimStatus::MissingTransport)return S::NeedsCommands;
  if(result.status==ArenaCommandClaimStatus::Invalid)return S::Invalid;
  if(result.status==ArenaCommandClaimStatus::Claimed){
   put(next,0x60,flags&~0x2000u);
   next[0xb4]=next[0x364];next[0xb5]=next[0x365];
   next[0xb2]=next[0x368];next[0xb3]=next[0x369];
  }
 }
 }
 else if(opcode==3){put(next,0x60,flags&~0x200000u);}
 else {
  const auto tail_flags=word(next,0x60);
  const bool timeout=word(next,0xbc)==1&&(tail_flags&0x200000u);put(next,0xbc,timeout?0:3);
  if(timeout){const auto slots=std::uint16_t(half(next,0xb0)+1);next[0xb0]=std::uint8_t(slots>>8);next[0xb1]=std::uint8_t(slots);
   put(next,0x60,(tail_flags&~0x200000u)|(next[15]==47?0x1000u:0u));}
 }
 fp(next,0xc4,timer);
 if(opcode==3)audio(ArenaAudioEvent{60,original_job_cell.x,original_job_cell.z});
 context=next;scratch=buffer;return S::Advanced;
}

S request_arena_ground_carry_pickup(ArenaGroundCarryContext& context,
 std::span<const std::uint8_t,0x2900> snapshot,ArenaAccumulatorRecord& scratch,
 const ArenaAccumulatorTransport& transport,ArenaCommandClaimResult* claim){
 if(context[15]<40||context[15]>48||word(context,0x500)!=context[15]||word(context,0x5d0)!=0x80)return S::Invalid;
 if(word(context,0x370))return S::NeedsCommands;
 const auto flags=word(context,0x60);
 if(!(flags&0x1000u)||!(word(context,0x160)&0x2000u)||(flags&0x200000u))return S::Advanced;
 if(!number(scalar(context,0x570)))return S::NeedsNumericRecovery;
 if(!(scalar(context,0x570)<bits(0x3ecccccc)))return S::Advanced;
 const auto peer=half(context,0x10c);if(peer>=32768)return S::Invalid;
 std::array<std::uint8_t,16> payload{};put(payload,0,2);
 payload[4]=context[0x4f2];payload[5]=context[0x4f3];
 payload[6]=context[0x10c];payload[7]=context[0x10d];
 const auto slot=std::uint16_t(20-half(context,0xb0));payload[8]=std::uint8_t(slot>>8);payload[9]=std::uint8_t(slot);
 payload[11]=1;payload[14]=0x10;
 auto buffer=scratch;
 const auto result=claim_arena_entity_command(buffer,word(snapshot,0x27f8),std::int16_t(peer),payload,transport);
 if(claim)*claim=result;
 if(result.status==ArenaCommandClaimStatus::MissingTransport)return S::NeedsCommands;
 if(result.status==ArenaCommandClaimStatus::Invalid)return S::Invalid;
 if(result.status==ArenaCommandClaimStatus::Claimed){
  const auto remaining=std::uint16_t(half(context,0xb0)-1);context[0xb0]=std::uint8_t(remaining>>8);context[0xb1]=std::uint8_t(remaining);
  put(context,0x60,(remaining?flags:flags&~0x1000u)|0x200000u);
 }
 scratch=buffer;return S::Advanced;
}

}
