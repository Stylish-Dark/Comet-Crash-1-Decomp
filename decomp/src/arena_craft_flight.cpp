#include "comet/arena_craft_flight.hpp"
#include "comet/arena_ground_contact.hpp"
#include <cfenv>
#include <bit>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
float normal(float v){return std::fpclassify(v)==FP_SUBNORMAL?0.f:v;}
float mul(float a,float b){volatile float v=normal(a)*normal(b);return normal(v);}
float sub(float a,float b){volatile float v=normal(a)-normal(b);return normal(v);}
float fused(float a,float b,float c){return normal(std::fma(normal(a),normal(b),normal(c)));}
struct Rounding {
 int previous=std::fegetround();
 Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("craft rounding unavailable");}
 ~Rounding(){std::fesetround(previous);}
};
ArenaCraftBoundaryContact advance(ArenaCraftFlightState& state,float x,float z,float elapsed,
 bool mode_flag,bool reduced_speed,const ArenaCraftBounds* bounds){
 if(!std::isfinite(x)||!std::isfinite(z)||std::abs(x)>1||std::abs(z)>1||
    !std::isfinite(elapsed)||elapsed<0||elapsed>.25f)throw std::invalid_argument("invalid craft flight input");
 for(const auto& v:{state.position,state.velocity})for(float f:v)
  if(!std::isfinite(f)||std::abs(f)>1000000)throw std::invalid_argument("unsupported craft flight state");
 Rounding rounding;auto next=state;
 // 104E0 selects the target velocity; 10530 advances with OLD velocity.
 std::array<float,4> desired{mul(x,mode_flag?10.f:40.f),0,mul(z,mode_flag?10.f:40.f),0};
 const float response=mode_flag?mul(elapsed,std::bit_cast<float>(0x40933333u)):1.f;
 for(unsigned i=0;i<4;++i){
  next.position[i]=fused(state.velocity[i],elapsed,state.position[i]);
 }
 ArenaCraftBoundaryContact contact;
 if(bounds){
  if(bounds->width<1||bounds->width>24||bounds->depth<1||bounds->depth>24)
   throw std::invalid_argument("invalid craft arena extent");
  ArenaGroundBoundsQuery q;q.position=next.position;q.velocity=next.velocity;
  for(auto& f:q.velocity)f=normal(f);
  q.max_x=float(bounds->width);q.max_z=float(bounds->depth);
  q.radius=bounds->radius;q.lateral_radius=bounds->lateral_radius;
  q.restitution_percent=bounds->restitution_percent;q.movable=bounds->movable;
  q.floor_uses_lateral_radius=bounds->floor_uses_lateral_radius;
  const auto bounded=contain_arena_ground_unit(q);
  if(!bounded)throw std::invalid_argument("unsupported craft arena contact");
  next.position=bounded->position;next.velocity=bounded->velocity;
  contact={bounded->accumulated_impulse,bounded->contacts,bounded->energetic_impact};
 }
 // 10548 containment updates the velocity read by 105B8/105E4.
 for(unsigned i=0;i<4;++i){
  if(mode_flag&&reduced_speed)desired[i]=mul(desired[i],std::bit_cast<float>(0x3f333333u));
  next.velocity[i]=fused(sub(desired[i],next.velocity[i]),response,next.velocity[i]);
 }
 // 105F0 clears the Y lane after integrating the old velocity.
 next.velocity[1]=0;
 for(const auto& v:{next.position,next.velocity})for(float f:v)
  if(!std::isfinite(f)||std::abs(f)>1000000)throw std::invalid_argument("craft flight overflow");
 state=next;return contact;
}
}
void advance_arena_craft_flight(ArenaCraftFlightState& state,float x,float z,float elapsed,bool mode_flag,bool reduced_speed){
 advance(state,x,z,elapsed,mode_flag,reduced_speed,nullptr);
}
ArenaCraftBoundaryContact advance_arena_craft_in_arena(ArenaCraftFlightState& state,float x,float z,float elapsed,
 const ArenaCraftBounds& bounds,bool mode_flag,bool reduced_speed){
 return advance(state,x,z,elapsed,mode_flag,reduced_speed,&bounds);
}
}
