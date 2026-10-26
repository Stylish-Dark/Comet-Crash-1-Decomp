#include "comet/arena_entity_finalization.hpp"
#include "comet/arena_player_craft.hpp"
#include "comet/arena_construction_simulation.hpp"
#include <cassert>
#include <bit>
#include <limits>
using namespace comet::decomp;
static unsigned word(const ArenaEntitySlot& s,unsigned o){return unsigned(s[o])<<24|unsigned(s[o+1])<<16|unsigned(s[o+2])<<8|s[o+3];}
static void put(ArenaEntitySlot& s,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)s[o+i]=v>>(24-8*i);}
static void fp(ArenaEntitySlot& s,unsigned o,float f){put(s,o,std::bit_cast<unsigned>(f));}
int main(){
 ArenaEntityStorage world;world.width=world.height=8;
 for(auto& b:world.banks)b.resize(4);
 reset_arena_entity_index_pool(world,4);
 ArenaEntitySlot source;source.fill(0xa5);
 initialize_arena_player_craft_entity(source,0,2.5f,3.5f,.125f);
 auto index=insert_arena_entity(world,{2,3},source);assert(index==0);
 source=world.banks[0][0];
 ArenaEntityFinalizationContext c;c.position_480={4.5f,1,3.5f,99};c.velocity_4a0={1,0,2,77};
 c.previous_scalar_38_4e8=1000;
 std::array<std::uint8_t,128> statistics{};
 const auto before_stats=statistics;
 assert(finalize_arena_entity_update(world,0,source,statistics,c,10)==ArenaEntityFinalization::Committed);
 const auto& moved=world.banks[0][0];
 assert(word(moved,0)==std::bit_cast<unsigned>(4.5f) && word(moved,4)==std::bit_cast<unsigned>(1.f));
 assert(word(moved,0xc)==word(source,0xc)); // identity/radius/kind, not position W
 assert(word(moved,0x4c)==word(source,0x4c)); // packed velocity W untouched
 assert(word(moved,0x40)==std::bit_cast<unsigned>(1.f) && word(moved,0x48)==std::bit_cast<unsigned>(2.f));
 assert((word(moved,0x60)&2)==0 && world.banks[0][0]==world.banks[1][0]);
 assert(world.cells[0][3*24+2].empty() && world.cells[0][3*24+4].size()==1);
 assert(statistics==before_stats);
 auto pending=world.banks[0][0];const auto retained_cells=world.cells;
 c.removal_590=1;
 assert(finalize_arena_entity_update(world,0,pending,statistics,c,10)==ArenaEntityFinalization::Tombstone);
 assert(world.banks[0][0][15]==63 && world.banks[0][0]==world.banks[1][0]);
 assert(word(world.banks[0][0],0x80)==13 && word(world.banks[0][0],0)==0xc1200000);
 assert(word(world.banks[0][0],4)==0xc1200000 && word(world.banks[0][0],8)==0);
 assert(world.free_indices.size()==3); // no immediate index reuse
 assert(world.summary_counts[3*24+4]==1);
 auto expected_cells=retained_cells;
 for(auto& b:expected_cells)b[3*24+4][0][3]=40;
 assert(world.cells==expected_cells); // no fabricated migration to cell zero
 // The outer wrapper tests final kind63 even when removal was already handled.
 c.removal_handled_593=1;auto tombstone=world.banks[0][0];
 assert(finalize_arena_entity_update(world,0,tombstone,statistics,c,11)==ArenaEntityFinalization::AlreadyHandled);
 assert(world.cells==expected_cells && word(world.banks[0][0],0x80)==13);
 c.removal_handled_593=0;
 assert(collect_arena_entity_tombstones(world,12).empty());
 assert(collect_arena_entity_tombstones(world,14).empty()); // exact serial, no >= shortcut
 auto due=collect_arena_entity_tombstones(world,13);assert((due==std::vector<std::uint16_t>{0}));
 assert(world.free_indices.size()==3); // collection does not mean job completed
 const auto snapshot=world;
 assert(!release_completed_arena_tombstones(world,due,12));
 assert(world.banks==snapshot.banks && world.free_indices==snapshot.free_indices);
 const std::array<std::uint16_t,2> duplicates{0,0};
 assert(!release_completed_arena_tombstones(world,duplicates,13));
 assert(world.banks==snapshot.banks && world.cells==snapshot.cells);
 assert(release_completed_arena_tombstones(world,due,13));
 assert((world.free_indices==std::deque<std::uint32_t>{1,2,3,0}));
 assert(world.cells[0][3*24+4].empty() && world.summary_counts[3*24+4]==0);
 assert(!release_completed_arena_tombstones(world,due,13));
 assert(world.banks[0][0]==snapshot.banks[0][0]); // released memory remains reused state

 // Removal already accounted for: clear common flag only, no motion/stats/deadline.
 auto handled=source;const auto original=handled;c.removal_handled_593=1;
 assert(finalize_arena_entity_record(handled,statistics,c,20)==ArenaEntityFinalization::AlreadyHandled);
 auto cleared=original;put(cleared,0x60,word(original,0x60)&~2u);assert(handled==cleared);
 assert(statistics==before_stats);
 // Unrecovered effects are a visible boundary, never silently skipped.
 c={};c.previous_scalar_38_4e8=1000;c.position_480={2.5f,0,3.5f,0};
 auto effect=source;fp(effect,0x38,999);const auto saved=effect;
 assert(finalize_arena_entity_record(effect,statistics,c,0)==ArenaEntityFinalization::NeedsChangeEffects);
 assert(effect==saved && statistics==before_stats);
 auto existing=source;fp(existing,0x38,999);existing[0x7f]=1;
 assert(finalize_arena_entity_record(existing,statistics,c,0)==ArenaEntityFinalization::Committed);
 assert(word(existing,0x38)==std::bit_cast<unsigned>(999.f)&&existing[0x7f]==1);
 c.previous_scalar_38_4e8=999;c.skip_spatial_6260=false;
 assert(finalize_arena_entity_record(effect,statistics,c,0)==ArenaEntityFinalization::NeedsSpatialRecovery);
 assert(effect==saved);
 c.skip_spatial_6260=true;c.position_480[0]=std::numeric_limits<float>::infinity();
 assert(finalize_arena_entity_record(effect,statistics,c,0)==ArenaEntityFinalization::Invalid);
 assert(effect==saved);
 // A real registered player body now accepts controller output through the
 // simulation API and keeps the player/root identity while it moves.
 ArenaConstructionSimulationConfig config;config.capacity=4;config.native_extent=8;
 config.players={{{{0,0},0,false},100,100}};
 ArenaConstructionSimulation simulation(config);ArenaEntitySlot scratch{};
 assert(simulation.create_player_craft(0,{2.5f,0,3.5f,0},scratch,
  [](const ArenaCraftAuxiliaryRequest&){return -1;})==ArenaCraftRegistration::Registered);
 c={};c.position_480={4.5f,1,3.5f,123};c.velocity_4a0={1,0,0,123};c.previous_scalar_38_4e8=1000;
 auto body=simulation.storage().banks[0][0];
 assert(simulation.finalize_player_craft(0,body,c,20)==ArenaEntityFinalization::Committed);
 assert(simulation.players()[0].craft_entity_index==0 && simulation.craft_registry().indices[0]==0);
 assert(word(simulation.storage().banks[0][0],0)==std::bit_cast<unsigned>(4.5f));
 assert(simulation.accounts()[0].resource==100);
 c.statistics_owner_542=1;
 assert(simulation.finalize_player_craft(0,body,c,20)==ArenaEntityFinalization::Invalid);
 // The stage5 removal path uses the recovered statistics operation exactly
 // once; a handled context must not decrement it again.
 auto ground=source;ground[15]=40;ground[0x33]=20;fp(ground,0x50,2);
 std::array<std::uint8_t,128> ground_stats{};
 ArenaUnitProductionQuota quota;quota.active=1;quota.capacity=10;
 store_arena_unit_production_quota(ground_stats,quota);
 assert(register_arena_entity_statistics(ground_stats,ground,true,false)==ArenaEntityRegistration::Registered);
 c={};c.removal_590=1;
 assert(finalize_arena_entity_record(ground,ground_stats,c,30)==ArenaEntityFinalization::Tombstone);
 assert(read_arena_unit_production_quota(ground_stats).active==0);
 const auto unregistered=ground_stats;c.removal_handled_593=1;
 assert(finalize_arena_entity_record(ground,ground_stats,c,30)==ArenaEntityFinalization::AlreadyHandled);
 assert(ground_stats==unregistered);
 // 32-bit original serial wraps, including the fixed +3 constructor.
 auto wrap=source;initialize_arena_entity_tombstone(wrap,0xfffffffeu);
 assert(word(wrap,0x80)==1 && arena_entity_tombstone_due(wrap,1));
 assert(!arena_entity_tombstone_due(wrap,0xfffffffeu));
 assert(wrap[0xc]==source[0xc] && wrap[0xd]==source[0xd] && word(wrap,0xd0)==word(source,0xd0));
 assert(word(wrap,0x38)==0x42c80000 && word(wrap,0x58)==0x3f800000 && word(wrap,0x64)==0x42c80000);
 assert(word(wrap,0x3c)==0 && wrap[0x33]==0 && word(wrap,0x60)==2);
}
