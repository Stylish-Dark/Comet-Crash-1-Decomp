#include "comet/arena_player_references.hpp"
#include "comet/arena_player_craft.hpp"
#include "comet/native_construction_scene.hpp"
#include <algorithm>
#include <bit>
#include <cassert>
using namespace comet::decomp;
namespace {
void put(auto& b,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)b[o+i]=v>>(24-8*i);}
void half(auto& b,unsigned o,unsigned v){b[o]=v>>8;b[o+1]=v;}
unsigned word(const auto& b,unsigned o){return unsigned(b[o])<<24|unsigned(b[o+1])<<16|unsigned(b[o+2])<<8|b[o+3];}
void fp(auto& b,unsigned o,float f){put(b,o,std::bit_cast<unsigned>(f));}
}
int main(){
 std::array<ArenaEntitySlot,4> bank;std::array<ArenaAccumulatorRecord,4> accum;
 for(unsigned n=0;n<4;++n){bank[n].fill(0x20+n);accum[n].fill(0x60+n);}
 ArenaEntityWorkContext work;work.fill(0xa5);work[15]=1;half(work,12,2);
 half(work,0x1c,0);half(work,0x1e,1);half(work,0xb8,3);
 ArenaPlayerReferenceCache cache;cache.attachment_tail.fill(0x8b);
 assert(fetch_arena_player_references(work,cache,bank,accum,7));
 assert(std::equal(bank[0].begin(),bank[0].end(),work.begin()+0x100));
 assert(std::equal(bank[1].begin(),bank[1].end(),work.begin()+0x200));
 assert(std::equal(accum[2].begin(),accum[2].end(),work.begin()+0x300));
 assert(std::equal(bank[3].begin(),bank[3].begin()+128,work.begin()+0x600));
 assert(std::equal(bank[3].begin()+128,bank[3].end(),cache.attachment_tail.begin()));
 assert(word(work,0x59c)==7&&work[0x380]==0xa5&&work[0x5d8]==0xa5);
 const auto before=work;const auto old_cache=cache;
 half(work,0x1e,0x8000);auto invalid=work;
 assert(!fetch_arena_player_references(work,cache,bank,accum,7)&&work==invalid&&cache.attachment_tail==old_cache.attachment_tail);
 work=before;half(work,0x1c,0xffff);half(work,0x1e,0xffff);half(work,0xb8,0xffff);
 auto sentinel=work;
 assert(fetch_arena_player_references(work,cache,bank,accum,3));
 assert(std::equal(sentinel.begin()+0x100,sentinel.begin()+0x300,work.begin()+0x100));
 assert(std::equal(sentinel.begin()+0x600,sentinel.end(),work.begin()+0x600));
 assert(cache.attachment_tail==old_cache.attachment_tail&&word(work,0x59c)==3);

 // Real registered peers -> B500 caches ->9A70 ->10290 ->6578 -> scene.
 ArenaState arena;arena.extent=8;arena.players.resize(2);
 arena.players[0].start_grid=ArenaGridPosition{0,0};arena.players[1].start_grid=ArenaGridPosition{1,1};
 for(auto& cell:arena.cells)cell.visibility_recovered=true;
 ArenaConstructionSimulationConfig config;config.capacity=2;config.native_extent=8;
 config.players={{{{0,0},0,false},100,100},{{{1,1},0,false},100,100}};
 NativeConstructionScene scene(arena,config);ArenaEntitySlot reused{};
 for(unsigned owner=0;owner<2;++owner)
  assert(scene.create_player_craft(owner,{owner?4.5f:2.5f,0,owner?2.5f:3.5f,0},reused,
    [](const ArenaCraftAuxiliaryRequest&){return -1;})==ArenaCraftRegistration::Registered);
 auto craft=scene.simulation().storage().banks[0][0];
 half(craft,0xb8,1);half(craft,0x1e,1);put(craft,0x60,word(craft,0x60)&~0x8000u);
 ArenaEntityFinalizationContext commit;commit.previous_scalar_38_4e8=std::bit_cast<float>(word(craft,0x38));
 commit.position_480={2.5f,0,3.5f,0};
 assert(scene.finalize_player_craft(0,craft,commit,1)==ArenaEntityFinalization::Committed);
 std::array<ArenaAccumulatorRecord,2> original_records{};
 for(auto& record:original_records){put(record,0x78,0xffffffff);fp(record,0x74,100);}
 ArenaPreparationSnapshot snapshot{};put(snapshot,0x28d8,8);put(snapshot,0x28dc,8);put(snapshot,0x2790,0xffffffff);
 ArenaEntityWorkContext connected{};ArenaPlayerReferenceCache connected_cache;
 for(unsigned frame=2;frame<=4;++frame){
  assert(scene.stage_player_craft_preparation(0,snapshot,connected,connected_cache,original_records,13));
  assert(std::equal(scene.simulation().storage().banks[0][1].begin(),scene.simulation().storage().banks[0][1].end(),connected.begin()+0x200));
  assert(word(connected,0x59c)==13);
  auto consumed=consume_arena_prepared_accumulator(connected,{});
  assert(scene.complete_player_craft_preparation(0,snapshot,consumed,connected)==ArenaPreparationCompletion::Prepared);
  auto result=scene.advance_prepared_player_craft(0,snapshot,.016f,connected,frame);
  assert(result.control.status==ArenaPlayerControlStatus::Advanced&&result.finalization==ArenaEntityFinalization::Committed);
  const auto& stored=scene.simulation().storage().banks[0][0];
  assert(word(stored,0xe0)==std::bit_cast<unsigned>(4.5f)&&word(stored,0xe8)==std::bit_cast<unsigned>(2.5f));
  assert(word(stored,0xec)==word(scene.simulation().storage().banks[0][1],12)); // metadata lane preserved
  assert(scene.simulation().storage().banks[0]==scene.simulation().storage().banks[1]);
  if(frame>2)assert(scene.arena().entities[0].position.x==4.5f&&scene.arena().entities[0].position.z==2.5f);
 }
 const auto saved=connected;const auto saved_cache=connected_cache;
 assert(!scene.stage_player_craft_preparation(0,snapshot,connected,connected_cache,{},13));
 assert(connected==saved&&connected_cache.attachment_tail==saved_cache.attachment_tail);
}
