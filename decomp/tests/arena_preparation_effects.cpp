#include "comet/arena_preparation_effects.hpp"
#include "comet/arena_player_craft.hpp"
#include "comet/arena_construction_simulation.hpp"
#include <algorithm>
#include <bit>
#include <cassert>
using namespace comet::decomp;
static void put(auto& b,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)b[o+i]=v>>(24-8*i);}
static void fp(auto& b,unsigned o,float v){put(b,o,std::bit_cast<unsigned>(v));}
static unsigned word(const auto& b,unsigned o){return unsigned(b[o])<<24|unsigned(b[o+1])<<16|unsigned(b[o+2])<<8|b[o+3];}
int main(){
 ArenaEntityWorkContext c;c.fill(0xa5);ArenaEntitySlot body{};
 initialize_arena_player_craft_entity(body,0,2.5f,3.5f);std::copy(body.begin(),body.end(),c.begin());
 ArenaPreparationSnapshot s{};put(s,0x28d8,8);put(s,0x28dc,8);
 std::array<std::uint8_t,128> statistics{};
 ArenaAccumulatorConsumption effects;effects.status=ArenaAccumulatorStatus::Cached;
 fp(effects.health_10,0,-20);fp(effects.impulse_40,0,7);
 // An actual staged context is supplied by the preceding recovered boundary.
 assert(stage_arena_entity_preparation(c,s));auto before=c;
 assert(complete_arena_entity_preparation(c,s,effects,statistics)==ArenaPreparationCompletion::Prepared);
 assert(word(c,0x38)==std::bit_cast<unsigned>(980.f) && word(c,0x4e8)==word(before,0x4e8));
 assert(word(c,0x4a0)!=word(before,0x4a0) && word(c,0x5d0)==0x80 && c[0x597]==0);
 auto death=before;death[0x34]=0;death[0x35]=1;fp(effects.health_10,0,-2000);const auto death_before=death;
 assert(complete_arena_entity_preparation(death,s,effects,statistics)==ArenaPreparationCompletion::MissingEmission);
 assert(death==death_before);
 ArenaEmissionDescriptor scratch;scratch.fill(0x5a);std::vector<ArenaCounterEmissionRequest> requests;
 assert(complete_arena_entity_preparation(death,s,effects,statistics,&scratch,
  [&](const ArenaCounterEmissionRequest& r){requests.push_back(r);})==ArenaPreparationCompletion::Prepared);
 assert(requests.size()==1 && requests[0].count==1 && requests[0].descriptor[15]==72);
 assert(death[0x34]==0 && death[0x35]==0);

 // Source3510 decomposition/order and full reused-byte handling.
 auto counter=before;counter[0x34]=0;counter[0x35]=27;scratch.fill(0x5a);requests.clear();
 auto blocked=counter;auto scratch_before=scratch;
 assert(!emit_arena_counter_requests(counter,7,true,scratch,{}) && counter==blocked && scratch==scratch_before);
 assert(emit_arena_counter_requests(counter,7,true,scratch,[&](const ArenaCounterEmissionRequest& r){requests.push_back(r);}));
 assert(requests.size()==4);
 const std::array<unsigned,4> counts{2,1,2,1};
 for(unsigned i=0;i<4;++i){assert(requests[i].count==counts[i]);assert(requests[i].descriptor[15]==(i<3?72+i:76));assert(word(requests[i].descriptor,0x60)==7);}
 assert(requests[0].descriptor[0x2c]==0x5a && requests[0].descriptor[0xc]==0);
 assert(requests[1].descriptor[0x2c]==0 && word(requests[0].descriptor,0x64)==0x5a5a5a5a);
 // Actual creation -> staging -> reserved effect consumption -> completion.
 ArenaConstructionSimulationConfig config;config.capacity=4;config.native_extent=8;
 config.players={{{{0,0},0,false},100,100}};ArenaConstructionSimulation simulation(config);
 assert(simulation.create_player_craft(0,{2.5f,0,3.5f,0},body,
   [](const ArenaCraftAuxiliaryRequest&){return -1;})==ArenaCraftRegistration::Registered);
 ArenaEntityWorkContext work{};assert(simulation.stage_player_craft_preparation(0,s,work));
 put(work,0x300,0x1000);fp(work,0x310,-5);fp(work,0x340,7);
 ArenaAccumulatorRecord shared{};put(shared,0,0x1000);fp(shared,0x10,-5);fp(shared,0x40,7);put(shared,0x78,0xffffffff);
 ArenaAccumulatorTransport transport;
 transport.read_reserved=[&](unsigned ea)->std::optional<ArenaAccumulatorRecord>{assert(ea==0x1000);return shared;};
 transport.write_conditional=[&](unsigned,const ArenaAccumulatorRecord& cleared){shared=cleared;return ArenaAccumulatorWrite::Committed;};
 auto consumed=consume_arena_prepared_accumulator(work,transport);const auto world=simulation.storage();
 assert(consumed.status==ArenaAccumulatorStatus::Consumed && word(shared,0x10)==0);

 assert(simulation.complete_player_craft_preparation(0,s,consumed,work)==ArenaPreparationCompletion::Prepared);
 assert(word(work,0x38)==std::bit_cast<unsigned>(995.f));
 assert(simulation.storage().banks==world.banks && simulation.accounts()[0].resource==100);
 const auto prepared=work;assert(simulation.complete_player_craft_preparation(1,s,consumed,work)==ArenaPreparationCompletion::Invalid && work==prepared);
 // Original full9A70 context fingerprints: threshold64, suppression, owner
 // gates, negative health, cap and reference relocation, with no emissions.
 constexpr std::array<std::uint64_t,8> hashes{
  0x96688c5174ed0e10ull,0x54f4114265409c1dull,0x59224a4eb9de9350ull,0x7ce93c318a931723ull,
  0xc85612575fa9f5bdull,0xda72be46a4f10663ull,0xddf23d62ccdd34b8ull,0xdbc5153802bf4af3ull};
 for(unsigned n=0;n<8;++n){
  ArenaEntityWorkContext context;ArenaPreparationSnapshot snapshot;std::uint32_t seed=0x52345678u+n;
  auto fill=[&](auto& b){for(auto& v:b){seed=1664525u*seed+1013904223u;v=std::uint8_t(seed>>24);}};
  fill(context);fill(snapshot);const unsigned owner=n%4;context[15]=1;context[0x33]=std::uint8_t((owner<<5)|24);
  context[0x34]=context[0x35]=0;
  const std::array<float,3> position{1.f+n,.5f,2.25f};
  for(unsigned i=0;i<3;++i)fp(context,4*i,position[i]);
  for(unsigned i=0;i<4;++i){fp(context,0x20+4*i,i==3?1.f:0.f);fp(context,0x40+4*i,.125f*i);
   fp(context,0x340+4*i,i==0?std::array<float,8>{0,8,8.000001f,30,-20,1,7,0}[n]:0);}
  put(context,0x300,0);fp(context,0x310,std::array<float,8>{-50,-500,0,100,-10,20,-300,1}[n]);
  fp(context,0x38,100.f+n);fp(context,0x64,200);fp(context,0x58,70);
  put(context,0x60,std::array<unsigned,8>{0x4002,0x4002,0x4002,0x100002,0x80002,2,0x800002,0x4002}[n]);
  put(snapshot,0x28d8,8);put(snapshot,0x28dc,8);
  for(unsigned i=0;i<4;++i){snapshot[0x28e0+i]=std::uint8_t(n%2);put(snapshot,0x2660+4*i,0);put(snapshot,0x2670+4*i,0);
   fp(snapshot,0x2780+4*i,n==5?100.f:.5f);}
  assert(stage_arena_entity_preparation(context,snapshot));auto consumption=consume_arena_prepared_accumulator(context,{});
  assert(complete_arena_entity_preparation(context,snapshot,consumption,statistics)==ArenaPreparationCompletion::Prepared);
  std::uint64_t hash=0xcbf29ce484222325ull;for(auto v:context)hash=(hash^v)*0x100000001b3ull;assert(hash==hashes[n]);
 }
}
