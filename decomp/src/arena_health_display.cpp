#include "comet/arena_health_display.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace comet::decomp { namespace {
unsigned word(const auto& b,unsigned o){return unsigned(b[o])<<24|unsigned(b[o+1])<<16|unsigned(b[o+2])<<8|b[o+3];}
void put(auto& b,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(24-8*i));}
void half(auto& b,unsigned o,unsigned v){b[o]=std::uint8_t(v>>8);b[o+1]=std::uint8_t(v);}
bool number(float v){return std::isfinite(v)&&std::fpclassify(v)!=FP_SUBNORMAL;}
struct Rounding{int previous=std::fegetround();Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("display rounding unavailable");}~Rounding(){std::fesetround(previous);}};
}
ArenaRequestPublicationResult publish_arena_health_display(ArenaRequestPublicationQueue& queue,
 ArenaEntitySlot& body,const ArenaHealthDisplayParameters& p,
 const std::array<float,4>& position,ArenaHealthDisplayScratch& scratch){
 if(body[0x7f])return {ArenaRequestPublicationStatus::Published,0}; //3994..39EC
 const auto capacity=queue.free_ring.size();
 if(!capacity||capacity%4||capacity>std::numeric_limits<std::uint32_t>::max()||queue.head>=capacity||queue.available>capacity||queue.records.size()>65536)return {};
 for(std::size_t i=0;i<queue.available;++i)if(queue.free_ring[(queue.head+i)%capacity]>=queue.records.size())return {};
 //2AD8 failure returns before reading the display's numeric inputs.
 if(queue.available<1026)return {ArenaRequestPublicationStatus::PoolFull,0};
 const bool player=(p.flags_520&24u)||p.kind_500==1;
 for(unsigned i=0;i<3;++i)if(!number(position[i]))return {};
 if(!player&&!number(p.offset_4e4))return {};
 Rounding rounding;float height=.7f;
 if(!player){volatile float v=p.offset_4e4+std::bit_cast<float>(p.kind_500>43?0x3eb33333u:0x3db851ebu);height=v;if(!number(height))return {};}
 const auto head=queue.head;queue.head=std::uint32_t((std::size_t(head)+2)%capacity);queue.available-=2;
 for(auto& lane:queue.availability_lanes)lane-=2;
 body[0x7f]=1; //3A30..3A48, only after successful reservation
 for(unsigned n=0;n<2;++n){
  const auto index=queue.free_ring[(std::size_t(head)+n)%capacity];auto& d=scratch[n];
  //1E480 fixed args: kind3, phaseFF, lifetime-1, delay3, velocity0,
  // source entity reference.14..1F,6C and7F retain reused stack bytes.
  for(unsigned i=0;i<3;++i)put(d,4*i,std::bit_cast<unsigned>(position[i]));
  half(d,12,index);d[14]=255;d[15]=std::uint8_t((player?105:101)+n);
  std::copy_n(d.begin(),16,d.begin()+0x20);put(d,0x10,3);
  put(d,0x30,0);put(d,0x34,std::bit_cast<unsigned>(height));put(d,0x38,player?0x3eb33333u:0);put(d,0x3c,0);
  std::fill(d.begin()+0x40,d.begin()+0x60,0);
  put(d,0x60,player?0x3f266666u:0x3eb33333u);put(d,0x64,word(body,0x38));
  put(d,0x68,0xbf800000);put(d,0x70,0xbf800000);put(d,0x74,0x40400000);put(d,0x78,n?0:1);
  half(d,0x7c,word(body,12)>>16);d[0x7e]=1;queue.records[index]=d;
 }
 return {ArenaRequestPublicationStatus::Published,2};
}
}
