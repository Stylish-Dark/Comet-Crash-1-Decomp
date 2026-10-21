#include "comet/arena_counter_emission.hpp"
#include <algorithm>
#include <bit>
namespace comet::decomp {
namespace {
void put(ArenaEmissionDescriptor& b,unsigned o,std::uint32_t v){for(unsigned i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(24-8*i));}
}
bool emit_arena_counter_requests(ArenaEntityWorkContext& context,std::uint32_t attribution,
 bool optional_request,ArenaEmissionDescriptor& reused,const ArenaCounterEmissionSink& sink){
 const unsigned counter=unsigned(context[0x34])<<8|context[0x35];
 const std::array<unsigned,3> counts{counter%5,(counter%10)/5,counter/10};
 if((counter||optional_request)&&!sink)return false;
 auto descriptor=reused;
 auto emit=[&](std::uint8_t model,unsigned count,float scalar){
  // The two position copies share pre-write metadata. +20 C/D survive;
  // +0 C/D are cleared. Keeping that distinction preserves reused bytes.
  std::copy_n(context.begin()+0x480,12,descriptor.begin());
  descriptor[14]=0;descriptor[15]=model;
  std::copy_n(descriptor.begin(),16,descriptor.begin()+0x20);
  descriptor[12]=descriptor[13]=0;
  put(descriptor,0x10,15);
  std::copy_n(context.begin()+0x4a0,16,descriptor.begin()+0x30);
  put(descriptor,0x60,attribution);put(descriptor,0x68,std::bit_cast<std::uint32_t>(scalar));
  descriptor[0x6c]=1;
  sink(ArenaCounterEmissionRequest{descriptor,count,0});
 };
 for(unsigned i=0;i<3;++i)if(counts[i])emit(std::uint8_t(72+i),counts[i],std::array<float,3>{1,5,10}[i]);
 if(optional_request)emit(76,1,0);
 context[0x34]=context[0x35]=0;reused=descriptor;return true;
}
}
