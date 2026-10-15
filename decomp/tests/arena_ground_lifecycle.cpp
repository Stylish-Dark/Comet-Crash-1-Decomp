#include "comet/arena_ground_lifecycle.hpp"
#include "comet/arena_ground_capture.hpp"
#include "comet/arena_player_statistics.hpp"
#include "comet/arena_unit_entity.hpp"
#include "comet/native_construction_scene.hpp"
#include <memory>
#include <cassert>
#include <bit>
#include <limits>
using namespace comet::decomp;
void put(ArenaEntitySlot&s,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)s[o+i]=v>>(24-8*i);}
float scalar(const ArenaEntitySlot&s,unsigned o){unsigned v=0;for(unsigned i=0;i<4;++i)v=(v<<8)|s[o+i];return std::bit_cast<float>(v);}
struct Fixture {
 ArenaEntitySlot slot{};std::array<std::uint8_t,128> stats{};
 Fixture(){ArenaRandom r(4);assert(initialize_arena_unit_template(slot,45,0,{3.5f,0,3.5f,0},r));ArenaUnitProductionQuota q;q.capacity=2;store_arena_unit_production_quota(stats,q);assert(register_arena_entity_statistics(stats,slot,true,true)==ArenaEntityRegistration::Registered);}
 void deplete(){put(slot,0x38,0);}
};
int main(){
 { // Real hostile contact -> death -> corpse -> expiry -> projection removal.
  ArenaConstructionSimulationConfig c;c.capacity=3;c.seed=4;
  c.players={{{{0,0},0,false},100,100},{{{10,10},1,false},100,100}};
  c.unit_base_available[0]=c.unit_base_available[1]=true;
  for(unsigned owner=0;owner<2;++owner){c.unit_quotas[owner].capacity=2;c.unit_quotas[owner].pending=1;c.unit_quotas[owner].by_type[5]=1;}
  c.ground_transport_enabled=c.ground_contacts_enabled=true;
  ArenaState arena;arena.players.resize(2);arena.players[0].start_grid=ArenaGridPosition{0,0};arena.players[1].start_grid=ArenaGridPosition{10,10};
  auto scene=std::make_unique<NativeConstructionScene>(arena,c);
  assert(scene->spawn_reserved_ground_unit(0,45,{3,3}).entity_index==0);
  assert(scene->spawn_reserved_ground_unit(1,45,{3,3}).entity_index==1);
  assert(scene->set_ground_capture_ready(1,true));
  const auto tick=scene->advance(.01f);assert(tick.depleted_units==std::vector<std::uint16_t>{0});
  assert(scene->simulation().unit_quotas()[0].active==0 && scene->simulation().unit_quotas()[1].active==1);
  assert(scene->simulation().storage().banks[0][0][15]==62 && scene->arena().entities[0].type==62);
  assert(scene->arena().entities.size()==2 && scene->simulation().storage().free_indices.size()==1);
  scene->advance(.11f);assert(scene->simulation().unit_quotas()[0].active==0);
  scene->advance(.21f);assert(scene->arena().entities[0].corpse_fraction && *scene->arena().entities[0].corpse_fraction<1);
  const auto expired=scene->advance(.51f);assert(expired.retired_entities==std::vector<std::uint16_t>{0});
  assert(scene->arena().entities.size()==1 && scene->arena().entities[0].live_entity_index==1);
  assert(scene->simulation().storage().free_indices==std::deque<std::uint32_t>({2,0}));
 }
 { // Migration updates both packed lists; full destinations roll back.
  auto storage=std::make_unique<ArenaEntityStorage>();for(auto &b:storage->banks)b.resize(2);reset_arena_entity_index_pool(*storage,2);
  Fixture f;assert(insert_arena_entity(*storage,{3,3},f.slot)==0);
  auto next=storage->banks[0][0];put(next,0,std::bit_cast<unsigned>(4.5f));
  storage->cells[1][76].resize(126);for(auto &entry:storage->cells[1][76])entry[1]=1;const auto before=storage->banks;
  assert(update_arena_entity(*storage,0,next)==ArenaEntityUpdate::CellFull);assert(storage->banks==before && storage->summary_counts[75]==1 && storage->cells[0][75].size()==1);
  storage->cells[1][76].clear();assert(update_arena_entity(*storage,0,next)==ArenaEntityUpdate::Updated);
  assert(storage->cells[0][75].empty() && storage->cells[0][76].size()==1 && storage->cells[0][76]==storage->cells[1][76]);
  assert(storage->summary_counts[75]==0 && storage->summary_counts[76]==1);
  assert(storage->banks[0]==storage->banks[1] && storage->free_indices==std::deque<std::uint32_t>{1});
  // A legal full destination suppresses drift, but cannot freeze expiry.
  auto corpse=storage->banks[0][0];put(corpse,0x38,0);assert(begin_arena_ground_death(corpse,f.stats)==ArenaGroundDeath::Corpse);
  put(corpse,0x40,std::bit_cast<unsigned>(10.f));put(corpse,0x90,std::bit_cast<unsigned>(10.f));
  assert(update_arena_entity(*storage,0,corpse)==ArenaEntityUpdate::Updated);
  for(auto &bank:storage->banks)bank.resize(128);
  for(unsigned index=2;index<128;++index)storage->free_indices.push_back(index);
  auto peer=f.slot;put(peer,0,std::bit_cast<unsigned>(5.5f));
  for(unsigned n=0;n<126;++n)assert(insert_arena_entity(*storage,{5,3},peer));
  assert(advance_arena_ground_corpse(*storage,0,.1f)==ArenaGroundCorpseStep::Active);
  assert(scalar(storage->banks[0][0],0)==4.5f && scalar(storage->banks[0][0],0x84)>0);
  assert(advance_arena_ground_corpse(*storage,0,.1f)==ArenaGroundCorpseStep::Active);
  assert(advance_arena_ground_corpse(*storage,0,.25f)==ArenaGroundCorpseStep::Expired);
  assert(storage->free_indices==std::deque<std::uint32_t>({127,0}));
 }

 {Fixture f;const auto before=f.slot;const auto stats=f.stats;assert(begin_arena_ground_death(f.slot,f.stats)==ArenaGroundDeath::Alive);assert(f.slot==before && f.stats==stats);}
 {Fixture f;f.deplete();put(f.slot,0x40,std::bit_cast<unsigned>(2.f));const auto original=f.slot;
  assert(begin_arena_ground_death(f.slot,f.stats)==ArenaGroundDeath::Corpse);
  assert(f.slot[15]==62 && (f.slot[0x33]&28)==0 && f.slot[0x6f]==original[0x6f]);
  assert(scalar(f.slot,0x80)==std::bit_cast<float>(0x3eccccccu) && scalar(f.slot,0x84)==0 && scalar(f.slot,0x8c)==.5f);
  assert(read_arena_unit_production_quota(f.stats).active==0);
  const auto corpse=f.slot;const auto stats=f.stats;assert(begin_arena_ground_death(f.slot,f.stats)==ArenaGroundDeath::Unsupported);assert(f.slot==corpse && f.stats==stats);
  assert(advance_arena_ground_corpse(f.slot,.1f)==ArenaGroundCorpseStep::Active);assert(scalar(f.slot,0)>3.69f && scalar(f.slot,0)<3.71f);
  assert(advance_arena_ground_corpse(f.slot,.1f)==ArenaGroundCorpseStep::Active);
  assert(scalar(f.slot,0x78)<scalar(corpse,0x78));
  assert(advance_arena_ground_corpse(f.slot,.25f)==ArenaGroundCorpseStep::Expired);
 }
 {Fixture f;f.deplete();put(f.slot,0x60,0x08000000);assert(begin_arena_ground_death(f.slot,f.stats)==ArenaGroundDeath::Retire);assert(read_arena_unit_production_quota(f.stats).active==0);}
 {Fixture f;f.deplete();put(f.slot,0x40,std::bit_cast<unsigned>(std::numeric_limits<float>::quiet_NaN()));const auto before=f.slot;const auto stats=f.stats;assert(begin_arena_ground_death(f.slot,f.stats)==ArenaGroundDeath::Invalid);assert(f.slot==before && f.stats==stats);}
 {Fixture f;f.deplete();assert(begin_arena_ground_death(f.slot,f.stats)==ArenaGroundDeath::Corpse);const auto before=f.slot;assert(advance_arena_ground_corpse(f.slot,0)==ArenaGroundCorpseStep::Invalid);assert(f.slot==before);put(f.slot,0x80,0);const auto malformed=f.slot;assert(advance_arena_ground_corpse(f.slot,.1f)==ArenaGroundCorpseStep::Invalid);assert(f.slot==malformed);}
 {Fixture f;f.deplete();ArenaUnitProductionQuota q;q.capacity=2;store_arena_unit_production_quota(f.stats,q);const auto before=f.slot;const auto stats=f.stats;assert(begin_arena_ground_death(f.slot,f.stats)==ArenaGroundDeath::Invalid);assert(f.slot==before && f.stats==stats);}
 {Fixture f;auto peer=f.slot;peer[0x33]|=32;peer[0xbb]=1;std::array<unsigned char,2> teams{0,1};std::array<ArenaPlayerStatisticsBlock,2> stats{f.stats,{}};
  assert(capture_arena_ground_unit(f.slot,peer,teams,stats,true)==ArenaGroundCapture::HealthDepleted);
  assert(scalar(f.slot,0x38)==0 && f.slot[0x33]>>5==0 && stats[0]==f.stats);
 }
}
