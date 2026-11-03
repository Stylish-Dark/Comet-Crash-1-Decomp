#include "comet/native_construction_scene.hpp"
#include "comet/arena_preparation_effects.hpp"
#include <algorithm>
#include <bit>
#include <cassert>
#include <vector>
using namespace comet::decomp;
namespace {
unsigned word(const auto& b,unsigned o){return unsigned(b[o])<<24|unsigned(b[o+1])<<16|unsigned(b[o+2])<<8|b[o+3];}
void put(auto& b,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(24-8*i));}
void fp(auto& b,unsigned o,float v){put(b,o,std::bit_cast<unsigned>(v));}
float scalar(const auto& b,unsigned o){return std::bit_cast<float>(word(b,o));}
ArenaPreparationSnapshot image(unsigned mode){
 ArenaPreparationSnapshot s{};put(s,0x28d8,8);put(s,0x28dc,8);put(s,0x2790,0xffffffff);
 put(s,0x2800,0x82000000);put(s,0x2770,mode==0?0xffffffffu:1);s[0x28e0]=mode==2;s[0x27b8]=1;
 fp(s,0x2560,.1f);fp(s,0x2564,.05f);return s;
}
ArenaEntityWorkContext prepared(const ArenaPreparationSnapshot& s){
 ArenaEntitySlot body{};initialize_arena_player_craft_entity(body,0,2.5f,3.5f);
 ArenaEntityWorkContext c{};std::copy(body.begin(),body.end(),c.begin());assert(stage_arena_entity_preparation(c,s));
 auto consumed=consume_arena_prepared_accumulator(c,{});ArenaAccumulatorRecord stats{};
 assert(complete_arena_entity_preparation(c,s,consumed,stats)==ArenaPreparationCompletion::Prepared);
 c[0xbc]=0;fp(c,0x38,-1);put(c,0x60,word(c,0x60)&~0x8000u);return c;
}
void connected(unsigned mode,bool exhausted){
 ArenaState arena;arena.extent=8;arena.players.resize(1);arena.players[0].start_grid=ArenaGridPosition{0,0};
 for(auto& cell:arena.cells)cell.visibility_recovered=true;
 ArenaConstructionSimulationConfig config;config.capacity=1;config.native_extent=8;
 config.players={{{{0,0},0,false},100,100}};config.base_counters[0]=75;
 NativeConstructionScene scene(arena,config);ArenaEntitySlot reused{};
 assert(scene.create_player_craft(0,{2.25f,0,3.25f,0},reused,[](const ArenaCraftAuxiliaryRequest&){return -1;})==ArenaCraftRegistration::Registered);
 auto s=image(mode);ArenaEntityWorkContext work{};assert(scene.stage_player_craft_preparation(0,s,work));
 // Supply the original ready BC0 state. The missing outer bootstrap is not
 // replaced here. Damage itself enters through an actual reserved accumulator.
 work[0xbc]=0;ArenaAccumulatorRecord damage{};put(damage,0,0x81000000);fp(damage,0x10,-2000);
 std::copy(damage.begin(),damage.end(),work.begin()+0x300);unsigned damage_reads=0,damage_writes=0;
 ArenaAccumulatorTransport accumulator;
 accumulator.read_reserved=[&](unsigned ea)->std::optional<ArenaAccumulatorRecord>{assert(ea==0x81000000);++damage_reads;return damage;};
 accumulator.write_conditional=[&](unsigned ea,const ArenaAccumulatorRecord& r){assert(ea==0x81000000);++damage_writes;damage=r;return ArenaAccumulatorWrite::Committed;};
 auto consumed=consume_arena_prepared_accumulator(work,accumulator);
 assert(consumed.status==ArenaAccumulatorStatus::Consumed&&damage_reads==1&&damage_writes==1&&scalar(damage,0x10)==0);
 assert(scene.complete_player_craft_preparation(0,s,consumed,work)==ArenaPreparationCompletion::Prepared&&scalar(work,0x38)<0);
 ArenaRequestPublicationQueue queue;queue.records.resize(256);queue.free_ring.resize(256);queue.available=exhausted?128:256;
 for(unsigned i=0;i<256;++i)queue.free_ring[i]=i;
 ArenaRandom random(19);ArenaParticleEmissionScratch particle_scratch;unsigned particle_calls=0,sounds=0;
 ArenaPlayerParticleTransport particles{&random,[&](const ArenaParticleEmission& request,ArenaRandom& rng){
  assert(sounds==1);++particle_calls;const auto result=publish_arena_particles(queue,request,rng,particle_scratch);
  assert(result.status==(exhausted?ArenaRequestPublicationStatus::PoolFull:ArenaRequestPublicationStatus::Published));
 }};
 auto audio=[&](const ArenaAudioEvent& e){assert(e.sound==7&&e.parameter_6==2&&e.parameter_7==3);++sounds;};
 ArenaPlayerPendingTransport full{[&](const ArenaEntitySlot&){assert(scene.simulation().storage().free_indices.empty());return -1;},[](const ArenaEntitySlot&){assert(false);}};
 ArenaPlayerControlScratch scratch;const ArenaGridPosition cell{2,3};
 const bool resume=mode==1&&exhausted;
 auto result=scene.advance_prepared_player_craft(0,s,.016f,work,17,&scratch,full,{},particles,audio,resume?nullptr:&queue,{},&cell);
 if(resume){
  assert(result.control.status==ArenaPlayerControlStatus::NeedsFinalization&&result.finalization==ArenaEntityFinalization::NeedsChangeEffects);
  const auto saved_statistics=scene.simulation().player_statistics();
  ArenaEntitySlot retained;std::copy_n(work.begin(),256,retained.begin());ArenaHealthDisplayScratch display_scratch{};
  result.finalization=scene.finalize_player_craft(0,retained,result.commit,17,{&queue,&display_scratch});
  assert(result.finalization==ArenaEntityFinalization::Committed&&scene.simulation().player_statistics()==saved_statistics);
 }else assert(result.control.status==ArenaPlayerControlStatus::Advanced);
 assert(sounds==1&&particle_calls==3);
 const auto& storage=scene.simulation().storage();assert(storage.banks[0]==storage.banks[1]);
 const auto& stats=scene.simulation().player_statistics()[0];
 assert((unsigned(stats[88])<<8|stats[89])==(mode==0?25u:75u));
 assert(queue.available==(exhausted?128u:192u));
 if(mode==1){
  assert(result.finalization==ArenaEntityFinalization::Committed&&work[15]==1&&work[0xbc]==1&&work[0x590]==0);
  assert(scalar(work,0x38)==1&&scalar(work,0xc0)==2&&scalar(work,0x74)==0&&scalar(work,0x78)==0);
  assert(scalar(work,0xd4)==0&&scalar(stats,116)==2&&stats[85]==1);
  assert(scene.arena().entities.size()==1&&scene.arena().entities[0].position.x==scalar(storage.banks[0][0],0));
  // The next original preparation/controller update continues the actual reset
  // body. It must not replay the destruction sound, particles or accounting.
  assert(scene.stage_player_craft_preparation(0,s,work));
  std::copy(damage.begin(),damage.end(),work.begin()+0x300);
  consumed=consume_arena_prepared_accumulator(work,{});
  assert(scene.complete_player_craft_preparation(0,s,consumed,work)==ArenaPreparationCompletion::Prepared);
  const auto next=scene.advance_prepared_player_craft(0,s,.016f,work,18,&scratch,full,{},particles,audio,&queue,{},&cell);
  assert(next.control.status==ArenaPlayerControlStatus::Advanced&&next.finalization==ArenaEntityFinalization::Committed);
  assert(sounds==1&&particle_calls==3&&scene.simulation().player_statistics()[0][85]==1);
 }else{
  assert(result.finalization==ArenaEntityFinalization::Tombstone&&work[15]==63&&storage.banks[0][0][15]==63);
  assert(word(storage.banks[0][0],0x80)==20&&stats[92]==255&&stats[85]==0);
  assert(scene.arena().entities.empty());
 }
 if(exhausted){ArenaRandom expected(19);expected.next();expected.next();assert(random.next()==expected.next());}
}
}
int main(){
 const ArenaGridPosition cell{2,3};
 for(unsigned mode=0;mode<3;++mode)for(bool exhausted:{false,true})connected(mode,exhausted);
 // Explicit finalizer commits must also remove the live renderer projection;
 // the original tombstone remains in both banks until its release frame.
 {
  ArenaState arena;arena.extent=8;arena.players.resize(1);arena.players[0].start_grid=ArenaGridPosition{0,0};
  for(auto& c:arena.cells)c.visibility_recovered=true;
  ArenaConstructionSimulationConfig config;config.capacity=2;config.native_extent=8;config.players={{{{0,0},0,false},100,100}};
  config.tuning[20]=ArenaStructureTableValues{100,50,10,20,7.5f};
  NativeConstructionScene scene(arena,config);ArenaEntitySlot reused{};
  assert(scene.create_player_craft(0,{2.25f,0,3.25f,0},reused,[](const ArenaCraftAuxiliaryRequest&){return -1;})==ArenaCraftRegistration::Registered);
  assert(scene.enqueue({0,20,4,4})==ArenaConstructionSubmit::Queued);
  assert(scene.step().status==ArenaConstructionStep::Constructed);
  assert(scene.arena().cell(4,4)->entity_index==1);
  const auto selection=scene.select_factory(0,{4,4},1);
  auto body=scene.simulation().storage().banks[0][0];ArenaEntityFinalizationContext commit;commit.removal_590=1;
  assert(scene.finalize_player_craft(0,body,commit,9)==ArenaEntityFinalization::Tombstone);
  assert(scene.arena().entities.size()==1&&scene.simulation().storage().banks[0][0][15]==63);
  assert(scene.arena().cell(4,4)->entity_index==0&&scene.arena().entities[0].live_entity_index==1);
  assert(scene.select_factory(0,{4,4},1)==selection);
  assert(scene.simulation().storage().banks[0]==scene.simulation().storage().banks[1]);
 }
 auto s=image(1),before=s;auto work=prepared(s);const auto initial=work;ArenaPlayerControlScratch scratch;
 ArenaRandom random(19);unsigned calls=0;ArenaPlayerParticleTransport particles{&random,[&](const ArenaParticleEmission&,ArenaRandom&){++calls;}};
 auto audio=[&](const ArenaAudioEvent&){++calls;};ArenaAccumulatorRecord stats{};fp(stats,116,3);stats[93]=17;
 unsigned reads=0,writes=0;ArenaAccumulatorTransport transport;
 transport.read_reserved=[&](unsigned ea)->std::optional<ArenaAccumulatorRecord>{assert(ea==0x82000000);++reads;return stats;};
 transport.write_conditional=[&](unsigned ea,const ArenaAccumulatorRecord& record){
  assert(ea==0x82000000);++writes;
  if(writes==1){fp(stats,116,5);stats[93]=19;return ArenaAccumulatorWrite::Retry;}
  if(writes==3){fp(stats,116,8);stats[93]=23;return ArenaAccumulatorWrite::Retry;}
  stats=record;return ArenaAccumulatorWrite::Committed;
 };
 assert(advance_arena_player_control(work,s,.016f,&scratch,{}, {},particles).status==ArenaPlayerControlStatus::NeedsAudio);
 assert(work==initial&&calls==0&&s==before);
 assert(advance_arena_player_control(work,s,.016f,&scratch,{}, {},particles,audio,{}, {},&cell).status==ArenaPlayerControlStatus::NeedsPlayerStatistics);
 assert(work==initial&&calls==0&&reads==0);
 assert(advance_arena_player_control(work,s,.016f,&scratch,{}, {},particles,audio,{},transport).status==ArenaPlayerControlStatus::NeedsJobCell);
 assert(work==initial&&calls==0&&reads==0);
 // BF58 reloads5 after losing its first reservation. BB30 loses another,
 // reloads8 and applies its fixed2-5 delta to that fresh record (result5).
 assert(advance_arena_player_control(work,s,.016f,&scratch,{}, {},particles,audio,{},transport,&cell).status==ArenaPlayerControlStatus::Advanced);
 assert(reads==4&&writes==4&&scalar(work,0xd4)==5&&scalar(stats,116)==5&&stats[93]==23&&stats[85]==1&&calls==4);
 // Permanent destruction in banking mode retires each real proxy identity,
 // preserving its constructor metadata and issuing no fresh allocation.
 s=image(0);put(s,0x28d4,1);fp(s,0x27b0,.016f);put(s,0x27c0,17);work=prepared(s);
 fp(work,0x94,1);fp(work,0xa4,1);
 for(unsigned i=0;i<4;++i){const unsigned o=std::array<unsigned,4>{0xc8,0xcc,0xca,0xce}[i];work[o]=0;work[o+1]=std::uint8_t(20+i);scratch.interaction_proxies[i][0x90]=std::uint8_t(70+i);}
 std::vector<ArenaEntitySlot> proxies;ArenaPlayerPendingTransport pending{[](const ArenaEntitySlot&)->int{assert(false);return -1;},[&](const ArenaEntitySlot& e){proxies.push_back(e);}};
 transport.write_conditional=[&](unsigned,const ArenaAccumulatorRecord& r){stats=r;return ArenaAccumulatorWrite::Committed;};
 assert(advance_arena_player_control(work,s,.016f,&scratch,pending,{},particles,audio,{},transport,&cell).status==ArenaPlayerControlStatus::Advanced);
 assert(proxies.size()==4);for(unsigned i=0;i<4;++i){assert(proxies[i][15]==63&&proxies[i][13]==20+i&&proxies[i][0x90]==70+i&&word(proxies[i],0x80)==20);}
 for(unsigned o:{0xc8u,0xccu,0xcau,0xceu})assert(work[o]==255&&work[o+1]==255);
}
