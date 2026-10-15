#include "comet/arena_ground_lifecycle.hpp"
#include "comet/arena_player_statistics.hpp"
#include "arena_spu_math.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
unsigned word(const ArenaEntitySlot &s,unsigned o){unsigned v=0;for(unsigned i=0;i<4;++i)v=(v<<8)|s[o+i];return v;}
float scalar(const ArenaEntitySlot &s,unsigned o){return std::bit_cast<float>(word(s,o));}
void put(ArenaEntitySlot &s,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)s[o+i]=v>>(24-8*i);}
void fp(ArenaEntitySlot &s,unsigned o,float v){put(s,o,std::bit_cast<unsigned>(v));}
bool number(float f){return std::isfinite(f) && std::fpclassify(f)!=FP_SUBNORMAL;}
float multiply(float a,float b){volatile float f=a*b;return f;}
float add(float a,float b){volatile float f=a+b;return f;}
struct Rounding {
 int previous=std::fegetround();
 Rounding(){if(previous<0 || std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("corpse rounding unavailable");}
 ~Rounding(){std::fesetround(previous);}
};
float divide(float a,float b){
 const float inverse=detail::reciprocal_estimate(b);
 const float quotient=multiply(a,inverse);
 return std::fma(std::fma(-quotient,b,a),inverse,quotient);
}
}
ArenaGroundDeath begin_arena_ground_death(ArenaEntitySlot &slot,std::span<std::uint8_t,128> statistics){
 const unsigned kind=slot[15];
 if(!(kind==40 || kind==41 || (kind>=43 && kind<=47)) || (slot[0x33]&28)!=20)return ArenaGroundDeath::Unsupported;
 const float health=scalar(slot,0x38);
 if(!number(health))return ArenaGroundDeath::Invalid;
 if(health>0)return ArenaGroundDeath::Alive;
 const bool direct=(word(slot,0x60)&0x08000000u)!=0;
 if(!direct){
  for(unsigned o:{0x40u,0x44u,0x48u,0x4cu,0x54u,0x78u})if(!number(scalar(slot,o)))return ArenaGroundDeath::Invalid;
 }
 auto next=slot;std::array<std::uint8_t,128> stats;std::copy(statistics.begin(),statistics.end(),stats.begin());
 if(read_arena_unit_production_quota(stats).active<=0)return ArenaGroundDeath::Invalid;
 try{if(register_arena_entity_statistics(stats,slot,false,true)!=ArenaEntityRegistration::Registered)return ArenaGroundDeath::Invalid;}
 catch(const std::invalid_argument&){return ArenaGroundDeath::Invalid;}
 if(!direct){
  std::copy_n(slot.begin()+0x40,16,next.begin()+0x90);
  put(next,0xa0,word(slot,0x54));put(next,0x88,word(slot,0x78));
  put(next,0x80,0x3eccccccu);put(next,0x84,0);put(next,0x8c,0x3f000000u);
  next[15]=62;next[0x33]&=0xe3;
 }
 put(next,0x60,word(next,0x60)&~2u); // 6578 common finalization
 slot=next;std::copy(stats.begin(),stats.end(),statistics.begin());
 return direct?ArenaGroundDeath::Retire:ArenaGroundDeath::Corpse;
}
ArenaGroundCorpseStep advance_arena_ground_corpse(ArenaEntitySlot &slot,float elapsed){
 if(slot[15]!=62)return ArenaGroundCorpseStep::Unsupported;
 if(!number(elapsed) || elapsed<=0 || elapsed>.25f)return ArenaGroundCorpseStep::Invalid;
 for(unsigned o:{0u,4u,8u,0x40u,0x44u,0x48u,0x4cu,0x80u,0x84u,0x88u,0x8cu,0x90u,0x94u,0x98u,0x9cu,0xa0u})
  if(!number(scalar(slot,o)))return ArenaGroundCorpseStep::Invalid;
 const float duration=scalar(slot,0x80),age=scalar(slot,0x84);
 if(duration<=0 || duration>10 || age<0 || age>10)return ArenaGroundCorpseStep::Invalid;
 Rounding rounding;auto next=slot;
 const float new_age=add(age,elapsed),fraction=divide(age,duration),remaining=add(1.f,-fraction);
 const float scale=std::max(std::bit_cast<float>(0x38d1b717u),multiply(remaining,scalar(slot,0x88)));
 const float scalar_54=add(scalar(slot,0xa0),divide(multiply(fraction,scalar(slot,0x8c)),scale));
 for(unsigned axis=0;axis<3;++axis)fp(next,4*axis,std::fma(scalar(slot,0x40+4*axis),elapsed,scalar(slot,4*axis)));
 for(unsigned axis=0;axis<4;++axis)fp(next,0x40+4*axis,multiply(scalar(slot,0x90+4*axis),remaining));
 fp(next,0x78,scale);fp(next,0x84,new_age);fp(next,0x54,scalar_54);
 for(unsigned o:{0u,4u,8u,0x40u,0x44u,0x48u,0x4cu,0x54u,0x78u,0x84u})if(!number(scalar(next,o)))return ArenaGroundCorpseStep::Invalid;
 slot=next;return new_age>duration?ArenaGroundCorpseStep::Expired:ArenaGroundCorpseStep::Active;
}
ArenaGroundCorpseStep advance_arena_ground_corpse(ArenaEntityStorage &storage,std::uint16_t index,float elapsed){
 if(storage.read_bank>1 || index>=storage.banks[0].size() || index>=storage.banks[1].size() ||
    storage.banks[0][index]!=storage.banks[1][index])return ArenaGroundCorpseStep::Invalid;
 auto next=storage.banks[0][index];
 const auto state=advance_arena_ground_corpse(next,elapsed);
 if(state==ArenaGroundCorpseStep::Expired)
  return retire_arena_entity(storage,index)?state:ArenaGroundCorpseStep::Invalid;
 if(state!=ArenaGroundCorpseStep::Active)return state;
 auto result=update_arena_entity(storage,index,next);
 if(result==ArenaEntityUpdate::CellFull){
  std::copy_n(storage.banks[0][index].begin(),12,next.begin());
  result=update_arena_entity(storage,index,next);
 }
 return result==ArenaEntityUpdate::Updated?state:ArenaGroundCorpseStep::Invalid;
}

}
