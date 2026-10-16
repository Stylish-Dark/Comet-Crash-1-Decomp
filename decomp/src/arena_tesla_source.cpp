#include "comet/arena_tesla_source.hpp"
#include "comet/arena_entity_core.hpp"
#include "arena_spu_math.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
namespace comet::decomp {
namespace {
unsigned word(const ArenaEntitySlot&s,unsigned o){unsigned v=0;for(unsigned i=0;i<4;i++)v=(v<<8)|s[o+i];return v;}
void word(ArenaEntitySlot&s,unsigned o,unsigned v){for(unsigned i=0;i<4;i++)s[o+i]=static_cast<std::uint8_t>(v>>(24-8*i));}
float scalar(const ArenaEntitySlot&s,unsigned o){return std::bit_cast<float>(word(s,o));}
void scalar(ArenaEntitySlot&s,unsigned o,float v){word(s,o,std::bit_cast<unsigned>(v));}
float add(float a,float b){volatile float v=a+b;return v;}
float subtract(float a,float b){volatile float v=a-b;return v;}
float multiply(float a,float b){volatile float v=a*b;return v;}
struct Rounding {int old;~Rounding(){std::fesetround(old);}};
bool tower(const ArenaEntitySlot&s){return s[15]==22&&((s[0x33]>>2)&7)==4;}
}
std::optional<ArenaTeslaSourceResult> advance_arena_tesla_source(
    ArenaEntitySlot&s,float elapsed,std::uint32_t commands,
    std::optional<std::uint16_t> target,bool exists){
 if(!tower(s)||s[0xba]!=0||s[0xbb]!=1||!std::isfinite(elapsed)||elapsed<0||elapsed>3||
    !std::isfinite(scalar(s,0xc8))||!std::isfinite(scalar(s,0xb4))||target==0xffff)return {};
 const int old=std::fegetround();if(old<0||std::fesetround(FE_TOWARDZERO))return {};
 Rounding rounding{old};
 float cooldown=scalar(s,0xc8);
 if(commands&&cooldown<0)cooldown=4;
 const bool ready=(s[0x60]&1)&&cooldown<0;
 ArenaTeslaSourceResult result;
 if(ready){
  result.spawn_beam=!exists;result.target=target;
  float sound_timer=add(scalar(s,0xb4),elapsed);
  if(s[0x90]==255&&s[0x91]==255&&target&&sound_timer>std::bit_cast<float>(0x3eccccccu))sound_timer=0;
  scalar(s,0xb4,sound_timer);scalar(s,0xc4,elapsed);
  const auto id=result.target.value_or(0xffff);
  s[0x90]=static_cast<std::uint8_t>(id>>8);s[0x91]=static_cast<std::uint8_t>(id);
 }
 s[0xb8]=0;s[0xb9]=ready&&exists?1:0;
 scalar(s,0xc8,subtract(cooldown,elapsed));
 return result;
}
bool initialize_arena_tesla_beam(ArenaEntitySlot&beam,const ArenaEntitySlot&source){
 if(!tower(source))return false;
 for(unsigned i=0;i<3;i++)if(!std::isfinite(scalar(source,4*i)))return false;
 const int old=std::fegetround();if(old<0||std::fesetround(FE_TOWARDZERO))return false;
 Rounding rounding{old};
 ArenaEntityCoreParameters p;
 p.type=5;p.model_slot=40;p.owner=source[0x33]>>5;p.quantity=5;
 for(unsigned i=0;i<3;i++)p.position[i]=scalar(source,4*i);
 p.position[1]=add(p.position[1],std::bit_cast<float>(0x3dccccccu));
 p.scalar_58=std::bit_cast<float>(0x3c23d70au);p.scalar_3c=60;
 p.scalar_38=p.scalar_64=std::bit_cast<float>(0x4e6e6b28u);
 initialize_arena_entity_core(std::span<std::uint8_t,128>(beam.data(),128),p);
 beam[0x1e]=source[0xc];beam[0x1f]=source[0xd];
 for(unsigned o:{0x80,0x90,0xa0,0xb0,0xc0})for(unsigned i=0;i<4;i++)scalar(beam,o+4*i,p.position[i]);
 word(beam,0xe0,0);word(beam,0xe4,0xffffffff);word(beam,0xe8,source[0x6e]);word(beam,0xec,0);
 word(beam,0xf0,0);word(beam,0xf8,0);beam[0xfc]=0;
 return true;
}
std::optional<float> arena_tesla_target_score(const ArenaPackedCellEntry&e,
    float distance,std::uint8_t route,std::uint16_t source_team,std::uint16_t candidate_team){
 const unsigned stage=(e[7]>>2)&7;
 if(!std::isfinite(distance)||distance<0||distance>64||source_team==candidate_team||stage<1||stage>5)return {};
 const int old=std::fegetround();if(old<0||std::fesetround(FE_TOWARDZERO))return {};
 Rounding rounding{old};
 const float near=subtract(32,distance);
 if(e[3]==36)return add(near,33);
 if(e[3]==0)return add(near,66);
 if(stage==5){
  const float priority=std::fma(-float(route>>4),3.f,48.f);
  return add(near,std::fma(float(e[7]&3),100.f,std::fma(priority,10000.f,110.f)));
 }
 return stage==4?near:10.f;
}
std::optional<float> arena_tesla_cell_distance(int dx,int dz){
 if(dx < -23||dx>23||dz < -23||dz>23||(dx==0&&dz==0))return {};
 const int old=std::fegetround();if(old<0||std::fesetround(FE_TOWARDZERO))return {};
 Rounding rounding{old};
 const float x=float(dx),z=float(dz),square=std::fma(x,x,multiply(z,z));
 const float inverse=detail::inverse_root_estimate(square),base=multiply(inverse,square);
 return std::fma(std::fma(-inverse,base,std::bit_cast<float>(0x3f800001u)),multiply(base,.5f),base);
}
} // namespace comet::decomp
