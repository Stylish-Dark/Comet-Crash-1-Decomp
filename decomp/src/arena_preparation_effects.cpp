#include "comet/arena_preparation_effects.hpp"
#include "arena_spu_math.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
std::uint32_t word(const auto& b,unsigned o){return std::uint32_t(b[o])<<24|std::uint32_t(b[o+1])<<16|std::uint32_t(b[o+2])<<8|b[o+3];}
float scalar(const auto& b,unsigned o){return std::bit_cast<float>(word(b,o));}
void put(ArenaEntityWorkContext& b,unsigned o,std::uint32_t v){for(unsigned i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(24-8*i));}
void fp(ArenaEntityWorkContext& b,unsigned o,float v){put(b,o,std::bit_cast<std::uint32_t>(v));}
bool number(float v){return std::isfinite(v)&&std::fpclassify(v)!=FP_SUBNORMAL;}
struct NumericBoundary{};
float checked(float v){if(!number(v))throw NumericBoundary{};return v;}
float mul(float a,float b){volatile float r=a*b;return checked(r);}
float add(float a,float b){volatile float r=a+b;return checked(r);}
float sub(float a,float b){volatile float r=a-b;return checked(r);}
float ma(float a,float b,float c){return checked(std::fma(a,b,c));}
float nm(float a,float b,float c){return checked(std::fma(a,-b,c));}
float dot3(const std::array<float,4>& v){return ma(v[2],v[2],ma(v[1],v[1],mul(v[0],v[0])));}
float inverse_root(float squared){
 if(squared<=0)throw NumericBoundary{};
 const float estimate=checked(detail::inverse_root_estimate(squared));
 return ma(nm(estimate,mul(estimate,squared),std::bit_cast<float>(0x3f800001u)),mul(estimate,.5f),estimate);
}
float root(float squared){
 if(squared<=0)throw NumericBoundary{};
 const float estimate=checked(detail::inverse_root_estimate(squared)),value=mul(estimate,squared);
 return ma(nm(estimate,value,std::bit_cast<float>(0x3f800001u)),mul(value,.5f),value);
}
struct Rounding{
 int previous=std::fegetround();
 Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("preparation rounding unavailable");}
 ~Rounding(){std::fesetround(previous);}
};
}
ArenaPreparationCompletion complete_arena_entity_preparation(ArenaEntityWorkContext& context,
 std::span<const std::uint8_t,0x2900> snapshot,const ArenaAccumulatorConsumption& effects,
 std::span<std::uint8_t,128> statistics,ArenaEmissionDescriptor* reused_descriptor,
 const ArenaCounterEmissionSink& emitter){
 if(effects.status!=ArenaAccumulatorStatus::Cached&&effects.status!=ArenaAccumulatorStatus::Consumed)
  return ArenaPreparationCompletion::Invalid;
 const unsigned owner=unsigned(context[0x542])<<8|context[0x543];
 if(owner>=4)return ArenaPreparationCompletion::Invalid;
 auto next=context;std::array<std::uint8_t,128> stats;std::copy(statistics.begin(),statistics.end(),stats.begin());
 const auto flags=word(next,0x60);Rounding rounding;bool emit_counter=false;
 try{
  if(!(flags&0x00100000u)){
   const float previous=scalar(next,0x38),maximum=scalar(next,0x64),delta=scalar(effects.health_10,0);
   if(!number(previous)||!number(maximum)||!number(delta))throw NumericBoundary{};
   const float sum=add(previous,delta);fp(next,0x38,maximum>sum?sum:maximum);
  }
  if(flags&0x4000u){
   std::array<float,4> impulse{};
   for(unsigned i=0;i<4;++i){impulse[i]=scalar(effects.impulse_40,4*i);if(!number(impulse[i])||!number(scalar(next,0x4a0+4*i)))throw NumericBoundary{};}
   const float squared=dot3(impulse);
   if(squared>64){ // strict source9F24 gate; cap XYZ norm at8, scale W too
    const float inverse=inverse_root(squared);
    for(auto& v:impulse)v=mul(mul(v,inverse),8);
   }
   const float mass=scalar(next,0x58);if(!number(mass)||mass==0)throw NumericBoundary{};
   const float estimate=checked(detail::reciprocal_estimate(mass)),initial=mul(1,estimate);
   const float inverse=ma(nm(initial,mass,1),estimate,initial);
   const float percent=mul(float(next[0x7d]),std::bit_cast<float>(0x3c23d70au));
   const float gain=mul(inverse,percent);
   for(unsigned i=0;i<4;++i)fp(next,0x4a0+4*i,ma(impulse[i],gain,scalar(next,0x4a0+4*i)));
  }
  next[0x597]=0;
  if(!(flags&0x00080000u)&&snapshot[0x28e0+owner]){
   next[0x597]=1;
   const float x=add(float(std::bit_cast<std::int32_t>(word(snapshot,0x2660+4*owner))),.5f);
   const float z=add(float(std::bit_cast<std::int32_t>(word(snapshot,0x2670+4*owner))),.5f);
   const float radius=scalar(snapshot,0x2780+4*owner);
   if(!number(radius))throw NumericBoundary{};
   std::array<float,4> difference{};
   for(unsigned i=0;i<3;++i){const float p=scalar(next,0x480+4*i);if(!number(p))throw NumericBoundary{};difference[i]=sub(p,i==0?x:(i==2?z:0.f));}
   const float distance=root(dot3(difference));
   if(radius>distance)fp(next,0x38,-1000); // strict sourceA1C4; no inferred meaning
  }
  if(!number(scalar(next,0x38)))throw NumericBoundary{};
  emit_counter=(next[0x34]||next[0x35])&&!(scalar(next,0x38)>0);
  if(emit_counter&&(!reused_descriptor||!emitter))return ArenaPreparationCompletion::MissingEmission;
  put(next,0x5d0,0x80); // source context+80 pointer becomes a relative PC work reference
  if((flags&2u)&&!(flags&0x00800000u)){
   register_arena_entity_statistics(stats,std::span<const std::uint8_t,0x680>(next).first<256>(),true,true,0);
   // SourceA248 ignores the result, including unsupported type1 accounting.
  }
 }catch(const NumericBoundary&){return ArenaPreparationCompletion::NeedsNumericRecovery;}
 catch(const std::invalid_argument&){return ArenaPreparationCompletion::Invalid;}
 if(emit_counter){
  // All rejecting calculations are complete before external requests. Source
  //3510 clears+34, thenA048/A218 stores the extension reference/statistics.
  auto scratch=*reused_descriptor;
  emit_arena_counter_requests(next,0xffffffffu,false,scratch,emitter);
  *reused_descriptor=scratch;
 }
 context=next;std::copy(stats.begin(),stats.end(),statistics.begin());return ArenaPreparationCompletion::Prepared;
}
}
