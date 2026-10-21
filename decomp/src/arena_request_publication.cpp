#include "comet/arena_request_publication.hpp"
#include "arena_spu_math.hpp"
#include <algorithm>
#include <cfenv>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace comet::decomp {
namespace {
struct Rounding {
 int previous=std::fegetround();
 Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("publication rounding unavailable");}
 ~Rounding(){std::fesetround(previous);}
};
float mul(float a,float b){volatile float v=a*b;return v;}
float add(float a,float b){volatile float v=a+b;return v;}
}
ArenaRequestPublicationResult publish_arena_request(ArenaRequestPublicationQueue& queue,
 const ArenaCounterEmissionRequest& request){
 if(!request.count)return {ArenaRequestPublicationStatus::Published,0};
 const auto capacity=queue.free_ring.size();
 if(!capacity||capacity%4||capacity>std::numeric_limits<std::uint32_t>::max()||
    queue.head>=capacity||queue.available>capacity||queue.records.size()>65536)
  return {};
 // Native bounds validation precedes mutation. Reserve entries are not read by
 // this call, but caller-owned FIFO identities must address the supplied pool.
 for(std::size_t i=0;i<queue.available;++i)
  if(queue.free_ring[(queue.head+i)%capacity]>=queue.records.size())return {};
 Rounding rounding;float phase=float(request.descriptor[14]),step=0;
 if(request.mode){
  volatile float count=float(request.count);
  const float estimate=detail::reciprocal_estimate(count),initial=mul(255,estimate);
  step=std::fma(std::fma(initial,-count,255.f),estimate,initial);phase=0;
 }
 std::uint32_t published=0;
 while(published<request.count){
  const auto batch=std::min(request.count-published,32u);
  if(queue.available<batch+128u)return {ArenaRequestPublicationStatus::PoolFull,published};
  const auto head=queue.head;
  queue.head=std::uint32_t((std::size_t(head)+batch)%capacity);queue.available-=batch;
  for(auto& lane:queue.availability_lanes)lane-=batch;
  for(unsigned i=0;i<batch;++i){
   const auto index=queue.free_ring[(std::size_t(head)+i)%capacity];
   auto descriptor=request.descriptor;
   descriptor[12]=std::uint8_t(index>>8);descriptor[13]=std::uint8_t(index);
   if(request.mode){
    const auto value=std::uint8_t(std::uint32_t(phase));
    descriptor[14]=value;descriptor[46]=value;
   }
   phase=add(phase,step);queue.records[index]=descriptor;
  }
  published+=batch;
 }
 return {ArenaRequestPublicationStatus::Published,published};
}
}
