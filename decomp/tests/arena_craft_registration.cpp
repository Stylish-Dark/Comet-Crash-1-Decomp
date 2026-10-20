#include "comet/arena_player_craft.hpp"
#include "comet/arena_construction_simulation.hpp"
#include <cassert>
#include <bit>
#include <limits>
#include <vector>
using namespace comet::decomp;

static std::uint32_t word(const ArenaEntitySlot& b,unsigned o){
 return std::uint32_t(b[o])<<24|std::uint32_t(b[o+1])<<16|std::uint32_t(b[o+2])<<8|b[o+3];
}
int main(){
 for(unsigned owner=0;owner<4;++owner){
  const auto spawn=resolve_arena_player_craft_spawn(owner,13,9,-1,999,.125f);
  assert(spawn && (*spawn)[0]==5.5f+float(owner%2) && (*spawn)[2]==3.5f+float(owner/2));
  assert((*spawn)[1]==0 && (*spawn)[3]==.125f);
 }
 const auto configured=resolve_arena_player_craft_spawn(3,13,9,0,7.25f,.5f);
 assert(configured && (*configured)[0]==0 && (*configured)[2]==7.25f);
 assert(!resolve_arena_player_craft_spawn(4,13,9,-1,0));
 assert(!resolve_arena_player_craft_spawn(0,0,9,-1,0));

 ArenaConstructionSimulationConfig config;
 config.capacity=5;
 config.players={{{{0,0},0,false},400,400},{{{7,7},1,false},300,300}};
 config.native_extent=8;
 ArenaConstructionSimulation simulation(config);
 std::vector<ArenaCraftAuxiliaryRequest> requests;
 ArenaEntitySlot reused;reused.fill(0xa5);
 const auto start=[&](const ArenaCraftAuxiliaryRequest& r)->std::int32_t{
  requests.push_back(r);
  // Source order: first call precedes body insertion; following calls see the
  // body and both player/root links. Handle +228 is written before call three.
  const auto& player=simulation.players()[1];
  if(requests.size()==1){assert(!player.craft_entity_index);assert(simulation.storage().free_indices.size()==5);return 19;}
  assert(player.craft_entity_index==0 && simulation.craft_registry().indices[1]==0);
  assert(simulation.storage().banks[0][0]==simulation.storage().banks[1][0]);
  assert(simulation.storage().free_indices.size()==4);
  if(requests.size()==2){assert(player.craft_auxiliary_handles[0]==-1);return 29;}
  assert(player.craft_auxiliary_handles[0]==29 && player.craft_auxiliary_handles[1]==-1);
  return 73;
 };
 assert(simulation.create_player_craft(1,{3.75f,0,4.25f,.125f},reused,start)==ArenaCraftRegistration::Registered);
 assert(requests.size()==3);
 assert((requests[0]==ArenaCraftAuxiliaryRequest{6,3,4,1,0}));
 assert(requests[1]==requests[0]);
 assert((requests[2]==ArenaCraftAuxiliaryRequest{5,3,4,1,0}));
 const auto& slot=simulation.storage().banks[0][0];
 assert(slot[15]==1 && slot[14]==24 && slot[0x33]==0x38);
 assert(word(slot,0)==std::bit_cast<std::uint32_t>(3.75f));
 assert(word(slot,4)==0 && word(slot,8)==std::bit_cast<std::uint32_t>(4.25f));
 assert(word(slot,0xec)==std::bit_cast<std::uint32_t>(.125f));
 assert(slot[0xc]==0 && slot[0xd]==0 && word(slot,0x80)==0xa5a5a5a5);
 assert(simulation.storage().summary_counts[4*24+3]==1);
 for(const auto& cells:simulation.storage().cells){
  const auto& entries=cells[4*24+3];assert(entries.size()==1);
  assert(entries[0][0]==0 && entries[0][1]==0 && entries[0][2]==24 && entries[0][3]==40 && entries[0][7]==0x38);
 }
 assert(simulation.players()[1].craft_auxiliary_handles[0]==29 && simulation.players()[1].craft_auxiliary_handles[1]==73);
 assert(!simulation.players()[0].craft_entity_index && !simulation.craft_registry().indices[0]);
 assert(simulation.accounts()[1].resource==300 && simulation.unit_quotas()[1].active==0);
 assert(simulation.create_player_craft(1,{2,0,2,0},reused,start)==ArenaCraftRegistration::AlreadyLinked);
 assert(requests.size()==3 && simulation.storage().free_indices.size()==4);
 // Native-owned checkpoint copies retain original body/player/root linkage.
 auto checkpoint=simulation;
 assert(checkpoint.players()[1].craft_entity_index==0 && checkpoint.craft_registry().indices[1]==0);
 assert(checkpoint.storage().banks==simulation.storage().banks);
 assert(checkpoint.players()[1].craft_auxiliary_handles[1]==73);

 // A second owner uses FIFO identity, updates only its links, and keeps any
 // negative auxiliary return instead of fabricating success or rolling back.
 unsigned calls=0;
 auto missing_voice=[&](const ArenaCraftAuxiliaryRequest&){++calls;return -1;};
 assert(simulation.create_player_craft(0,{.5f,0,.5f,0},reused,missing_voice)==ArenaCraftRegistration::Registered);
 assert(calls==3 && simulation.players()[0].craft_entity_index==1 && simulation.craft_registry().indices[0]==1);
 assert(simulation.players()[0].craft_auxiliary_handles[0]==-1 && simulation.players()[0].craft_auxiliary_handles[1]==-1);
 assert(simulation.players()[1].craft_entity_index==0);

 // The original missing-position branch places four owners around the arena
 // center. It does not use each owner's base cell or the preview's .85 height.
 ArenaConstructionSimulation centered(config);
 std::vector<ArenaCraftAuxiliaryRequest> centered_requests;
 auto centered_audio=[&](const ArenaCraftAuxiliaryRequest& r){centered_requests.push_back(r);return -1;};
 assert(centered.create_player_craft(0,{-1,0,999,.25f},reused,centered_audio)==ArenaCraftRegistration::Registered);
 assert(centered.create_player_craft(1,{-1,0,999,.5f},reused,centered_audio)==ArenaCraftRegistration::Registered);
 assert(word(centered.storage().banks[0][0],0)==std::bit_cast<std::uint32_t>(3.5f));
 assert(word(centered.storage().banks[0][0],8)==std::bit_cast<std::uint32_t>(3.5f));
 assert(word(centered.storage().banks[0][1],0)==std::bit_cast<std::uint32_t>(4.5f));
 assert(word(centered.storage().banks[0][1],8)==std::bit_cast<std::uint32_t>(3.5f));
 assert(word(centered.storage().banks[0][1],4)==0 && word(centered.storage().banks[0][1],0xec)==std::bit_cast<std::uint32_t>(.5f));
 assert((centered_requests[0]==ArenaCraftAuxiliaryRequest{6,3,3,1,0}));
 assert((centered_requests[3]==ArenaCraftAuxiliaryRequest{6,4,3,1,0}));
 ArenaEntityStorage storage;storage.width=storage.height=8;
 for(auto& bank:storage.banks)bank.resize(2);
 reset_arena_entity_index_pool(storage,2);
 ArenaPlayer player;ArenaPlayerCraftRegistry registry;
 const auto unchanged=[&]{assert(!player.craft_entity_index && !registry.indices[0]);assert(storage.free_indices.size()==2);assert(storage.summary_counts[0]==0);assert(calls==3);};
 assert(register_arena_player_craft(storage,player,registry,0,{.5f,0,.5f,0},reused,{})==ArenaCraftRegistration::MissingAuxiliary);unchanged();
 for(const auto& spawn: {std::array<float,4>{-.1f,0,.5f,0}, {8,0,.5f,0}, {.5f,1,.5f,0}, {.5f,0,.5f,std::numeric_limits<float>::quiet_NaN()}}){
  assert(register_arena_player_craft(storage,player,registry,0,spawn,reused,missing_voice)==ArenaCraftRegistration::Invalid);unchanged();
 }
 assert(register_arena_player_craft(storage,player,registry,4,{.5f,0,.5f,0},reused,missing_voice)==ArenaCraftRegistration::Invalid);unchanged();
 registry.indices[0]=1;
 assert(register_arena_player_craft(storage,player,registry,0,{.5f,0,.5f,0},reused,missing_voice)==ArenaCraftRegistration::AlreadyLinked);
 registry.indices[0].reset();
 storage.cells[1][0].resize(126);
 assert(register_arena_player_craft(storage,player,registry,0,{.5f,0,.5f,0},reused,missing_voice)==ArenaCraftRegistration::NoCapacity);unchanged();
 storage.cells[1][0].clear();storage.free_indices.clear();
 assert(register_arena_player_craft(storage,player,registry,0,{.5f,0,.5f,0},reused,missing_voice)==ArenaCraftRegistration::NoCapacity);
 assert(calls==3 && !player.craft_entity_index);
}
