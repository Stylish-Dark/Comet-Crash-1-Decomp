#include "comet/arena_entity_accumulator.hpp"
#include <algorithm>
#include <bit>
#include <cassert>
using namespace comet::decomp;
static void put(auto& b,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)b[o+i]=v>>(24-8*i);}
static unsigned word(const auto& b,unsigned o){return unsigned(b[o])<<24|unsigned(b[o+1])<<16|unsigned(b[o+2])<<8|b[o+3];}
int main(){
 ArenaAccumulatorRecord idle{};put(idle,0,0x1000);put(idle,0x78,0xffffffff);
 auto cached=consume_arena_entity_accumulator(idle,{});
 assert(cached.status==ArenaAccumulatorStatus::Cached && cached.attempts==0);
 auto local=idle;put(local,0x10,std::bit_cast<unsigned>(-5.f));const auto before=local;
 assert(consume_arena_entity_accumulator(local,{}).status==ArenaAccumulatorStatus::MissingTransport && local==before);
 ArenaAccumulatorRecord shared;for(unsigned i=0;i<128;++i)shared[i]=std::uint8_t(3*i+7);
 put(shared,0,0x1000);put(shared,0x10,std::bit_cast<unsigned>(-25.f));
 put(shared,0x40,std::bit_cast<unsigned>(1.f));put(shared,0x44,std::bit_cast<unsigned>(2.f));put(shared,0x48,std::bit_cast<unsigned>(3.f));
 auto fresh=shared;unsigned reads=0,writes=0;
 ArenaAccumulatorTransport transport;
 transport.read_reserved=[&](std::uint32_t ea)->std::optional<ArenaAccumulatorRecord>{assert(ea==0x1000);++reads;return shared;};
 transport.write_conditional=[&](std::uint32_t ea,const ArenaAccumulatorRecord& cleared){
  assert(ea==0x1000);++writes;
  if(writes==1){put(shared,0x10,std::bit_cast<unsigned>(-40.f));fresh=shared;return ArenaAccumulatorWrite::Retry;}
  shared=cleared;return ArenaAccumulatorWrite::Committed;
 };
 auto result=consume_arena_entity_accumulator(local,transport);
 assert(result.status==ArenaAccumulatorStatus::Consumed && result.attempts==2 && reads==2 && writes==2);
 assert(local==fresh && word(result.health_10,0)==std::bit_cast<unsigned>(-40.f));
 assert(word(result.impulse_40,0)==std::bit_cast<unsigned>(1.f) && result.request_30==word(fresh,0x30));
 auto expected=fresh;
 for(unsigned o:{0x10,0x20,0x40,0x50})std::fill_n(expected.begin()+o,16,0);
 std::fill_n(expected.begin()+0x30,16,255);
 for(unsigned o:{4,8,0x60,0x70})put(expected,o,0);
 expected[0xc]=expected[0xd]=expected[0xe]=expected[0xf]=0;
 put(expected,0x74,0x42c80000);put(expected,0x78,0xffffffff);
 assert(shared==expected); // all other shared bytes, including metadata, preserved
 ArenaEntityWorkContext work;work.fill(0xa5);std::copy(before.begin(),before.end(),work.begin()+0x300);
 writes=1;auto surrounding=work;
 assert(consume_arena_prepared_accumulator(work,transport).status==ArenaAccumulatorStatus::Consumed);
 assert(std::equal(surrounding.begin(),surrounding.begin()+0x300,work.begin()));
 assert(std::equal(surrounding.begin()+0x380,surrounding.end(),work.begin()+0x380));
 // Public full-result fingerprints from original instructions, including
 // local/shared records, the partial request output and all returned lanes.
 constexpr std::array<std::uint64_t,8> expected_hash{
  0x4d2e32b09d15d6c8ull,0x4121a690c78a8210ull,0x12051001eb67d4f9ull,0x3173a0f40060ac38ull,
  0x6061fc252fa85f71ull,0xc48bca1f0af5efd0ull,0x0511796fbd5853dbull,0x88313a1c152e1fc5ull};
 for(unsigned n=0;n<8;++n){
  std::uint32_t seed=0x2a345678u+n;ArenaAccumulatorRecord l,remote;
  auto fill=[&](auto& b){for(auto& v:b){seed=1664525u*seed+1013904223u;v=std::uint8_t(seed>>24);}};
  fill(l);fill(remote);put(l,0,n%4==0?0:0x1000);
  for(auto b:{&l,&remote}){
   put(*b,0x10,std::bit_cast<unsigned>(-30.5f+2*n));
   const std::array<float,3> impulse{float(n),.5f,-float(n)};
   for(unsigned i=0;i<3;++i)put(*b,0x40+4*i,std::bit_cast<unsigned>(impulse[i]));
  }
  if(n%4==2){for(unsigned o:{4,8,0x10,0x40,0x44,0x48,0x50,0x60,0x70})put(l,o,0);put(l,0x78,0xffffffff);}
  unsigned failures=n%3;ArenaAccumulatorTransport bus;
  bus.read_reserved=[&](unsigned)->std::optional<ArenaAccumulatorRecord>{return remote;};
  bus.write_conditional=[&](unsigned,const ArenaAccumulatorRecord& cleared){
   if(failures){--failures;return ArenaAccumulatorWrite::Retry;}
   remote=cleared;return ArenaAccumulatorWrite::Committed;
  };
  const auto consumed=consume_arena_entity_accumulator(l,bus);
  assert(consumed.status==ArenaAccumulatorStatus::Cached || consumed.status==ArenaAccumulatorStatus::Consumed);
  std::uint64_t hash=0xcbf29ce484222325ull;
  auto digest=[&](const auto& b){for(auto v:b)hash=(hash^v)*0x100000001b3ull;};
  digest(l);std::array<std::uint8_t,16> request;request.fill(0xa5);put(request,0,consumed.request_30);
  digest(request);digest(consumed.impulse_40);digest(consumed.health_10);digest(remote);
  assert(hash==expected_hash[n]);
 }
}
