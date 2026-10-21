#include "comet/arena_entity_preparation.hpp"
#include "comet/arena_player_craft.hpp"
#include "comet/arena_construction_simulation.hpp"
#include <algorithm>
#include <cassert>
#include <bit>
using namespace comet::decomp;
static void put(auto& b,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)b[o+i]=v>>(24-8*i);}
static unsigned word(const auto& b,unsigned o){return unsigned(b[o])<<24|unsigned(b[o+1])<<16|unsigned(b[o+2])<<8|b[o+3];}
int main(){
 ArenaEntityWorkContext c;c.fill(0xa5);ArenaEntitySlot body;body.fill(0xa5);
 initialize_arena_player_craft_entity(body,2,2.5f,3.5f);
 std::copy(body.begin(),body.end(),c.begin());
 ArenaPreparationSnapshot s{};put(s,0x28d8,8);put(s,0x28dc,6);
 s[0x2725]=2;s[(3*24+2)*16+3]=4;
 const auto before=c;
 assert(stage_arena_entity_preparation(c,s));
 assert(std::equal(body.begin(),body.end(),c.begin()));
 assert(std::equal(body.begin(),body.begin()+16,c.begin()+0x480));
 assert(word(c,0x490)==word(body,0) && word(c,0x494)==0);
 assert(word(c,0x4b8)==0xbf800000 && word(c,0x4bc)==0);
 assert(word(c,0x4e8)==0x447a0000 && word(c,0x4f0)==0xa5a5);
 assert(word(c,0x500)==1 && word(c,0x510)==6 && word(c,0x520)==64);
 assert(c[0x542]==0 && c[0x543]==2 && c[0x544]==0 && c[0x545]==2);
 assert(c[0x552]==0 && c[0x553]==2 && c[0x562]==0 && c[0x563]==3);
 assert(c[0x595]==1 && c[0x596]==0 && c[0x59a]==0 && word(c,0x5a0)==1);
 assert(std::equal(before.begin()+0x300,before.begin()+0x480,c.begin()+0x300));
 for(unsigned o=0;o<16;++o)assert(c[0x4d0+o]==0);
 auto invalid=before;put(s,0x28d8,0);
 assert(!stage_arena_entity_preparation(invalid,s) && invalid==before);
 // A normal input can produce subnormal intermediate arithmetic. Reject that
 // unsupported domain transactionally rather than imply SPU hardware parity.
 put(s,0x28d8,8);auto tiny=before;
 for(unsigned i=0;i<4;++i)put(tiny,0x20+4*i,std::bit_cast<unsigned>(1e-20f));
 const auto tiny_before=tiny;
 assert(!stage_arena_entity_preparation(tiny,s) && tiny==tiny_before);
 // Creation -> actual registered-body staging, with no world/health mutation.
 ArenaConstructionSimulationConfig config;config.capacity=4;config.native_extent=8;
 config.players={{{{0,0},0,false},100,100}};
 ArenaConstructionSimulation simulation(config);ArenaEntitySlot scratch{};
 assert(simulation.create_player_craft(0,{2.5f,0,3.5f,0},scratch,
   [](const ArenaCraftAuxiliaryRequest&){return -1;})==ArenaCraftRegistration::Registered);
 put(s,0x28d8,8);put(s,0x28dc,8);ArenaEntityWorkContext staged;staged.fill(0x6b);
 const auto storage_before=simulation.storage();
 assert(simulation.stage_player_craft_preparation(0,s,staged));
 assert(word(staged,0x4f0)==0 && word(staged,0x510)==6);
 assert(std::equal(storage_before.banks[0][0].begin(),storage_before.banks[0][0].end(),staged.begin()));
 assert(simulation.storage().banks==storage_before.banks && simulation.accounts()[0].resource==100);
 auto unchanged=staged;put(s,0x28dc,7);
 assert(!simulation.stage_player_craft_preparation(0,s,staged) && staged==unchanged);
 assert(!simulation.stage_player_craft_preparation(1,s,staged) && staged==unchanged);
 // Full-context fingerprints captured from untouched original9A70..9EC0.
 // Deterministic synthetic inputs; no original executable/data bytes embedded.
 constexpr std::array<std::uint64_t,8> expected{
  0x1a7380c66bc47fb7ull,0x88780402692a1058ull,0x819381f25b57756bull,0xe880d73630e0e8f2ull,
  0xbceeadf754064d1cull,0x19f21a3066f65006ull,0xd3cbf94cb68282d8ull,0x6939097004520e52ull};
 for(unsigned n=0;n<8;++n){
  std::uint32_t seed=0x12345678u+n;ArenaEntityWorkContext work;ArenaPreparationSnapshot snap;
  auto fill=[&](auto& b){for(auto& v:b){seed=1664525u*seed+1013904223u;v=std::uint8_t(seed>>24);}};
  fill(work);fill(snap);const unsigned owner=n%4;
  work[0x33]=std::uint8_t((owner<<5)|((n%8)<<2)|(n%4));
  put(snap,0x28d8,7+n);put(snap,0x28dc,6+n);
  const std::array<float,3> position{-2.75f+7.25f*n,.5f*n,30.5f-6.25f*n};
  const std::array<float,4> quaternion{.125f*n,-.25f*n,.5f*(n%3),1-.125f*n};
  for(unsigned i=0;i<3;++i)put(work,4*i,std::bit_cast<unsigned>(position[i]));
  for(unsigned i=0;i<4;++i)put(work,0x20+4*i,std::bit_cast<unsigned>(quaternion[i]));
  put(work,0x60,std::array<unsigned,4>{0,0x80000,0x4000,0x100000}[n%4]);
  const auto visibility=std::array<unsigned,4>{0,2,64,255}[n%4];
  snap[0x2720+2*owner]=std::uint8_t(visibility>>8);snap[0x2721+2*owner]=std::uint8_t(visibility);
  assert(stage_arena_entity_preparation(work,snap));
  std::uint64_t hash=0xcbf29ce484222325ull;
  for(auto v:work)hash=(hash^v)*0x100000001b3ull;
  assert(hash==expected[n]);
 }
}
