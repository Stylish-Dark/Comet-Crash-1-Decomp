#include "comet/arena_player_references.hpp"
#include <algorithm>
namespace comet::decomp {
namespace {
unsigned half(const ArenaEntityWorkContext& b,unsigned o){return unsigned(b[o])<<8|b[o+1];}
}
bool fetch_arena_player_references(ArenaEntityWorkContext& context,ArenaPlayerReferenceCache& cache,
 std::span<const ArenaEntitySlot> bank,std::span<const ArenaAccumulatorRecord> accumulators,
 std::uint32_t tag){
 if(context[15]!=1)return false;
 const auto self=half(context,12),peer=half(context,0x1c),target=half(context,0x1e),attached=half(context,0xb8);
 if(self>=accumulators.size())return false;
 for(const auto index:{peer,target,attached})
  if(index!=65535&&(index>=32768||index>=bank.size()))return false;
 auto next=context;auto next_cache=cache;
 std::copy(accumulators[self].begin(),accumulators[self].end(),next.begin()+0x300);
 if(peer!=65535)std::copy(bank[peer].begin(),bank[peer].end(),next.begin()+0x100);
 if(target!=65535)std::copy(bank[target].begin(),bank[target].end(),next.begin()+0x200);
 if(attached!=65535){
  std::copy_n(bank[attached].begin(),128,next.begin()+0x600);
  std::copy_n(bank[attached].begin()+128,128,next_cache.attachment_tail.begin());
 }
 for(unsigned i=0;i<4;++i)next[0x59c+i]=std::uint8_t(tag>>(24-8*i));
 context=next;cache=next_cache;return true;
}
}
