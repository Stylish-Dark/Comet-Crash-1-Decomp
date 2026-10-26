#include "comet/arena_audio_event.hpp"
#include <cassert>
using namespace comet::decomp;
unsigned word(const auto& b){return unsigned(b[0])<<24|unsigned(b[1])<<16|unsigned(b[2])<<8|b[3];}
void count(auto& b,unsigned v){for(unsigned i=0;i<4;++i)b[i]=std::uint8_t(v>>(24-8*i));}
int main(){
 ArenaAudioBatch batch;for(unsigned i=0;i<128;++i)batch[i]=i;count(batch,0);
 assert(append_arena_audio_event(batch,{0xabcd,7,9})==ArenaAudioAppend::Appended);
 std::uint64_t hash=14695981039346656037ull;for(auto b:batch)hash=(hash^b)*1099511628211ull;
 assert(hash==0xc671fde7cd4935daull&&word(batch)==1);
 // Reservation loss reloads a changed packet and never appends to stale bytes.
 ArenaAudioBatch first{},second{};first.fill(0x55);second.fill(0x66);count(first,27);count(second,3);
 unsigned reads=0,writes=0;ArenaAudioBatch committed{};
 ArenaAccumulatorTransport transport{
  [&](std::uint32_t ea)->std::optional<ArenaAccumulatorRecord>{assert(ea==0x12340000);return reads++?second:first;},
  [&](std::uint32_t ea,const ArenaAccumulatorRecord& data){assert(ea==0x12340000);
   if(++writes==1){assert(word(data)==28);return ArenaAccumulatorWrite::Retry;}
   committed=data;return ArenaAccumulatorWrite::Committed;}};
 assert(submit_arena_audio_event(batch,0x12340000,{8,0,0},transport)==ArenaAudioAppend::Appended);
 assert(reads==2&&writes==2&&batch==committed&&word(batch)==4);
 assert(batch[16]==0&&batch[17]==8&&batch[18]==0&&batch[19]==0&&batch[20]==0x66);
 // The source still performs a conditional write for a full packet.
 count(second,28);reads=writes=0;
 transport.read_reserved=[&](std::uint32_t)->std::optional<ArenaAccumulatorRecord>{++reads;return second;};
 transport.write_conditional=[&](std::uint32_t,const ArenaAccumulatorRecord& data){++writes;assert(data==second);return ArenaAccumulatorWrite::Committed;};
 assert(submit_arena_audio_event(batch,0x12340000,{9,2,3},transport)==ArenaAudioAppend::Full);
 assert(reads==1&&writes==1&&batch==second);
 const auto before=batch;
 assert(submit_arena_audio_event(batch,0x12340000,{8,0,0},{})==ArenaAudioAppend::MissingTransport&&batch==before);
}
