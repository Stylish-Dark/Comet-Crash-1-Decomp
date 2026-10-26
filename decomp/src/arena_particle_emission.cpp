#include "comet/arena_particle_emission.hpp"
#include "comet/arena_ground_pair.hpp"
#include "arena_spu_math.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace comet::decomp { namespace {
using V=std::array<float,4>;
float bits(unsigned v){return std::bit_cast<float>(v);}
unsigned word(const auto& b,unsigned o){return unsigned(b[o])<<24|unsigned(b[o+1])<<16|unsigned(b[o+2])<<8|b[o+3];}
void put(auto& b,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(24-8*i));}
void fp(auto& b,unsigned o,float v){put(b,o,std::bit_cast<unsigned>(v));}
void half(auto& b,unsigned o,unsigned v){b[o]=std::uint8_t(v>>8);b[o+1]=std::uint8_t(v);}
void store(auto& b,unsigned o,const V& v){for(unsigned i=0;i<4;++i)fp(b,o+4*i,v[i]);}
float mul(float a,float b){volatile float v=a*b;return v;}
float add(float a,float b){volatile float v=a+b;return v;}
float sub(float a,float b){volatile float v=a-b;return v;}
float ma(float a,float b,float c){return std::fma(a,b,c);}
float nm(float a,float b,float c){return std::fma(-a,b,c);}
float dot(const V& v){return ma(v[2],v[2],ma(v[1],v[1],mul(v[0],v[0])));}
float divide(float a,float b){const float e=detail::reciprocal_estimate(b),v=mul(a,e);return ma(nm(v,b,a),e,v);}
float inverse(float v){const float e=detail::inverse_root_estimate(v);return ma(nm(e,mul(e,v),bits(0x3f800001)),mul(e,.5f),e);}
float draw(ArenaRandom& random){volatile float v=float(random.next());return divide(v,bits(0x4e7fffff));}
struct Trig{float sine,cosine;};
Trig trig(float angle){
 const float n=add(mul(angle,bits(0x3f22f983)),std::copysign(.5f,angle));const int quadrant=int(n);
 const float reduced=nm(float(quadrant),bits(0x33a22168),nm(float(quadrant),bits(0x3fc90fd9),angle));
 const float s=mul(reduced,reduced);
 const float cosine=ma(ma(ma(s,bits(0xbab24992),bits(0x3d2aa035)),s,bits(0xbeffffdf)),s,1);
 const float sine=ma(ma(ma(s,bits(0xb94c8c6d),bits(0x3c088342)),s,bits(0xbe2aaaa1)),mul(s,reduced),reduced);
 auto select=[&](int q){const float v=q&1?cosine:sine;return q&2?-v:v;};
 return {select(quadrant),select(quadrant+1)};
}
// 1F2F8: random circle perpendicular to the supplied direction. Near-Y/zero
// directions retain original fallback bases (+Z,+X); no extra draw is used.
V circle(const V& direction,ArenaRandom& random){
 const V yzx{direction[1],direction[2],direction[0],direction[3]},
         zxy{direction[2],direction[0],direction[1],direction[3]};
 V cross{},second{},a{0,0,1,0},b{1,0,0,0};
 const V cy{0,0,1,0},cz{1,0,0,0};
 for(unsigned i=0;i<4;++i)cross[i]=nm(zxy[i],cz[i],mul(yzx[i],cy[i]));
 if(!(bits(0x38d1b717)>dot(cross))){
  const V c_yzx{cross[1],cross[2],cross[0],cross[3]},c_zxy{cross[2],cross[0],cross[1],cross[3]};
  for(unsigned i=0;i<4;++i)second[i]=nm(c_zxy[i],yzx[i],mul(c_yzx[i],zxy[i]));
  const float ia=inverse(dot(cross)),ib=inverse(dot(second));
  for(unsigned i=0;i<4;++i){a[i]=mul(second[i],ib);b[i]=mul(cross[i],ia);}
 }
 const auto t=trig(ma(draw(random),bits(0x40c90fda),0.f));V result{};
 for(unsigned i=0;i<4;++i)result[i]=ma(b[i],t.sine,mul(a[i],t.cosine));
 return result;
}
struct Rounding{int previous=std::fegetround();Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("particle rounding unavailable");}~Rounding(){std::fesetround(previous);}};
bool number(float v){return std::isfinite(v)&&std::fpclassify(v)!=FP_SUBNORMAL&&std::abs(v)<=1e6f;}
// 1E480 fixed r5=r9=r13=0,r10=FFFF,r11=4, followed by1EB68..1EBC8.
void construct(ArenaEmissionDescriptor& d,unsigned index,unsigned model,const V& position,
 const V& velocity,float lifetime,const ArenaParticleEmission& request){
 for(unsigned i=0;i<3;++i)fp(d,4*i,position[i]);half(d,12,index);d[14]=0;d[15]=std::uint8_t(model);
 std::copy_n(d.begin(),16,d.begin()+0x20);put(d,0x10,4);
 store(d,0x30,velocity);store(d,0x40,position);std::fill_n(d.begin()+0x50,16,0);
 fp(d,0x64,lifetime);fp(d,0x68,sub(lifetime,0));fp(d,0x70,lifetime);put(d,0x74,0);
 half(d,0x7c,65535);d[0x7e]=1;
 const float minimum=bits(word(request.configuration,0)),fade=sub(lifetime,bits(0x3d4ccccc));
 fp(d,0x14,minimum>fade?fade:minimum);put(d,0x18,word(request.configuration,4));
 put(d,0x78,request.configuration[12]);d[0x7f]=request.configuration[11];
}
}
ArenaRequestPublicationResult publish_arena_particles(ArenaRequestPublicationQueue& queue,
 const ArenaParticleEmission& request,ArenaRandom& random,ArenaParticleEmissionScratch& scratch){
 if(!request.count)return {ArenaRequestPublicationStatus::Published,0};
 const auto capacity=queue.free_ring.size();
 if(!capacity||capacity%4||capacity>std::numeric_limits<std::uint32_t>::max()||queue.head>=capacity||queue.available>capacity||queue.records.size()>65536||!request.configuration[9])return {};
 for(std::size_t i=0;i<queue.available;++i)if(queue.free_ring[(queue.head+i)%capacity]>=queue.records.size())return {};
 for(float v:{request.radius_min,request.radius_max,request.speed,request.lifetime_min,request.lifetime_max,bits(word(request.configuration,0)),bits(word(request.configuration,4))})if(!number(v))return {};
 for(const auto* v:{&request.position,&request.velocity})for(unsigned i=0;i<4;++i){
  if(i<3?!number((*v)[i]):(!std::isfinite((*v)[i])||std::abs((*v)[i])>1e6f))return {};
 }
 Rounding rounding;unsigned published=0;
 while(published<request.count){
  const auto batch=std::min(request.count-published,32u);
  if(queue.available<batch+128u)return {ArenaRequestPublicationStatus::PoolFull,published};
  const auto head=queue.head;queue.head=std::uint32_t((std::size_t(head)+batch)%capacity);queue.available-=batch;
  for(auto& lane:queue.availability_lanes)lane-=batch;
  for(unsigned i=0;i<batch;++i){
   const auto index=queue.free_ring[(std::size_t(head)+i)%capacity];V position=request.position,velocity{};
   if(request.configuration[10]){
    const auto direction=circle(request.velocity,random);for(unsigned j=0;j<4;++j)velocity[j]=mul(direction[j],request.speed);
   }else{
    const auto direction=arena_ground_random_direction(random);
    for(unsigned j=0;j<4;++j)velocity[j]=add(request.velocity[j],mul(direction[j],request.speed));
    const float radius=ma(sub(request.radius_max,request.radius_min),draw(random),request.radius_min);
    for(unsigned j=0;j<4;++j)position[j]=ma(direction[j],radius,request.position[j]);
   }
   const float lifetime=ma(sub(request.lifetime_max,request.lifetime_min),draw(random),request.lifetime_min);
   const unsigned model=request.configuration[8]+random.next()%request.configuration[9];
   auto& record=scratch.records[i];construct(record,index,model,position,velocity,lifetime,request);queue.records[index]=record;
  }
  published+=batch;
 }
 return {ArenaRequestPublicationStatus::Published,published};
}
}
