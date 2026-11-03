#include "comet/arena_player_lifecycle.hpp"
#include "comet/arena_player_statistics.hpp"
#include "comet/arena_ground_pair.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {namespace {
unsigned word(const auto& b,unsigned o){return unsigned(b[o])<<24|unsigned(b[o+1])<<16|unsigned(b[o+2])<<8|b[o+3];}
unsigned half(const auto& b,unsigned o){return unsigned(b[o])<<8|b[o+1];}
void put(auto& b,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(24-8*i));}
float scalar(const auto& b,unsigned o){return std::bit_cast<float>(word(b,o));}
void fp(auto& b,unsigned o,float v){put(b,o,std::bit_cast<unsigned>(v));}
bool number(float f){return std::isfinite(f)&&std::fpclassify(f)!=FP_SUBNORMAL&&std::abs(f)<=1e6f;}
struct Rounding {int old=std::fegetround();Rounding(){if(old<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("player death rounding unavailable");}~Rounding(){std::fesetround(old);}};
void update(ArenaAccumulatorRecord& scratch,unsigned address,
 const ArenaAccumulatorTransport& transport,const std::function<void(ArenaAccumulatorRecord&)>& change){
 for(;;){
  const auto loaded=transport.read_reserved(address);if(!loaded)throw std::runtime_error("original player statistics reservation unavailable");
  auto next=*loaded;change(next);
  const auto result=transport.write_conditional(address,next);
  if(result==ArenaAccumulatorWrite::Unavailable)throw std::runtime_error("original player statistics publication unavailable");
  if(result==ArenaAccumulatorWrite::Committed){scratch=next;return;}
 }
}
}
bool prepare_arena_player_death(ArenaEntityWorkContext& work,
 std::span<const std::uint8_t,0x2900> snapshot,ArenaPlayerDeathScratch& scratch,ArenaPlayerDeathEffects& effects){
 const auto owner=half(work,0x542);
 if(work[15]!=1||word(work,0x500)!=1||word(work,0x5d0)!=0x80||owner>=4||!work[0x590])return false;
 for(unsigned i=0;i<3;++i)if(!number(scalar(work,0x480+4*i)))return false;
 if(!std::isfinite(scalar(work,0x48c)))return false;
 auto next=work;auto staged=scratch;ArenaPlayerDeathEffects out;
 next[0xbc]=1;put(next,0xb0,0);next[0x1e]=next[0x1f]=255;next[0xbe]=0;
 out.without_base=std::bit_cast<std::int32_t>(word(snapshot,0x2770+4*owner))<0;
 out.reset=!out.without_base&&!snapshot[0x28e0+owner];
 if(out.reset){
  next[0x590]=0;fp(next,0x38,1);put(next,0x74,0);put(next,0x78,0);
  put(next,0xc0,0x40000000);next[0x33]&=0xe3;
  next[0xd0]=next[0x552];next[0xd1]=next[0x553];next[0xd2]=next[0x562];next[0xd3]=next[0x563];
 }
 for(unsigned n=0;n<3;++n){
  auto& request=out.particles[n];auto& cfg=staged.configurations[n];
  for(unsigned i=0;i<4;++i)request.position[i]=scalar(work,0x480+4*i);
  put(cfg,0,n?0x3c23d70a:0x3d0b4395);put(cfg,4,0xc0000000);
  cfg[8]=std::uint8_t(n==0?40:n==1?43:48);cfg[9]=1;cfg[10]=n?1:0;cfg[11]=1;cfg[12]=n?0:1;
  request.configuration=cfg;request.count=n?16:32;
  request.radius_max=n?0:std::bit_cast<float>(0x3dcccccc);
  request.speed=n==0?std::bit_cast<float>(0x3fb33333):n==1?5.f:4.f;
  request.lifetime_min=.5f;request.lifetime_max=std::bit_cast<float>(n==0?0x3f8ccccc:n==1?0x3f199999:0x3f4ccccc);
  if(n==1)request.velocity={0,1,0,0};
 }
 work=next;scratch=staged;effects=out;return true;
}
void publish_arena_player_death(ArenaEntityWorkContext& work,
 std::span<const std::uint8_t,0x2900> snapshot,ArenaPlayerDeathScratch& scratch,const ArenaPlayerDeathEffects& effects,
 ArenaRandom& random,const std::function<void(const ArenaParticleEmission&,ArenaRandom&)>& particles,
 const ArenaAudioEventSink& audio,const ArenaAccumulatorTransport& statistics,ArenaGridPosition cell){
 const auto owner=half(work,0x542),base=word(snapshot,0x2800);
 if(owner>=4||!base||(base&127u)||std::uint64_t(base)+128u*owner>0xffffffffu||
    !particles||!audio||!statistics.read_reserved||!statistics.write_conditional)
  throw std::invalid_argument("incomplete original player death transport");
 audio(ArenaAudioEvent{7,cell.x,cell.z});
 particles(effects.particles[0],random);particles(effects.particles[1],random);
 auto third=effects.particles[2];third.velocity=arena_ground_random_direction(random);particles(third,random);
 const auto address=base+128u*owner;
 if(effects.reset||effects.without_base){
  ArenaPlayerResourceDelta resource;resource.count_84=effects.reset?1:0;
  resource.base_counter=effects.without_base?-50:0;resource.base_counter_enabled=snapshot[0x27b8+owner]!=0;resource.attribution=65535;
  update(scratch.statistics,address,statistics,[&](auto& record){apply_arena_player_resource_delta(record,resource);});
 }
 ArenaPlayerStatisticsDelta delta;
 if(effects.reset){
  const float previous=scalar(scratch.statistics,116);if(!number(previous))throw std::runtime_error("unsupported original reset statistics");
  fp(work,0xd4,previous);Rounding rounding;volatile float difference=2.f-previous;delta.scalar_116=difference;
 }else delta.count_92=255;
 update(scratch.statistics,address,statistics,[&](auto& record){apply_arena_player_statistics_delta(record,delta);});
}
}
