#include "comet/arena_health_display.hpp"
#include <cassert>
#include <bit>
using namespace comet::decomp;
unsigned word(const auto& b,unsigned o){return unsigned(b[o])<<24|unsigned(b[o+1])<<16|unsigned(b[o+2])<<8|b[o+3];}
void put(auto& b,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(24-8*i));}
unsigned long long hash(const auto& b){unsigned long long h=14695981039346656037ull;for(auto v:b)h=(h^v)*1099511628211ull;return h;}
int main(){
 ArenaRequestPublicationQueue q;q.records.resize(1040);q.free_ring.resize(1040);q.available=1026;q.head=1039;
 for(unsigned i=0;i<1040;++i)q.free_ring[i]=i;
 q.free_ring[1039]=11;q.free_ring[0]=12;q.availability_lanes={3,4,5};
 ArenaEntitySlot body{};put(body,12,0x00050001);put(body,0x38,0x41000000);
 ArenaHealthDisplayScratch sc;for(unsigned n=0;n<2;++n)for(unsigned i=0;i<128;++i)sc[n][i]=std::uint8_t((i+128*n)*3+7);
 const auto original=sc;ArenaHealthDisplayParameters p;p.kind_500=1;
 auto result=publish_arena_health_display(q,body,p,{2.5f,0,3.5f,0},sc);
 assert(result.status==ArenaRequestPublicationStatus::Published&&result.published==2);
 assert(body[0x7f]==1&&q.available==1024&&q.head==1);
 assert((q.availability_lanes==std::array<std::uint32_t,3>{1,2,3}));
 assert(hash(sc[0])==0xfe81b19d231f8898ull&&hash(sc[1])==0x242fc47ff1c4418full);
 assert(q.records[11]==sc[0]&&q.records[12]==sc[1]);
 auto saved=q;auto saved_sc=sc;
 result=publish_arena_health_display(q,body,p,{9,9,9,9},sc);
 assert(result.published==0&&q.records==saved.records&&q.available==saved.available&&sc==saved_sc);
 // One short of source2+1024 reserve must not set the flag or touch scratch.
 body[0x7f]=0;q.available=1025;sc=original;
 result=publish_arena_health_display(q,body,p,{2.5f,0,3.5f,0},sc);
 assert(result.status==ArenaRequestPublicationStatus::PoolFull&&result.published==0);
 assert(!body[0x7f]&&sc==original&&q.available==1025&&q.head==1);
 // Non-player models depend on cached520 mask24 and unsigned kind>43.
 for(unsigned kind:{0u,2u,43u,44u,0xffffffffu})for(unsigned mask:{0u,8u,16u}){
  q=saved;q.available=1026;body[0x7f]=0;sc=original;p={kind,mask,.5f};
  result=publish_arena_health_display(q,body,p,{2.5f,0,3.5f,0},sc);assert(result.published==2);
  assert(sc[0][15]==(mask?105:101)&&sc[1][15]==(mask?106:102));
  assert(word(sc[0],0x60)==(mask?0x3f266666u:0x3eb33333u));
  assert(word(sc[0],0x34)==(mask?0x3f333333u:(kind>43?0x3f599999u:0x3f170a3du)));
 }
}
