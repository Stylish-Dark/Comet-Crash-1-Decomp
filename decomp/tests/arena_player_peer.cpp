#include "comet/arena_player_peer.hpp"
#include "comet/native_construction_scene.hpp"
#include <algorithm>
#include <bit>
#include <cassert>
using namespace comet::decomp;
unsigned word(const auto& b,unsigned o){return unsigned(b[o])<<24|unsigned(b[o+1])<<16|unsigned(b[o+2])<<8|b[o+3];}
void put(auto& b,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(24-8*i));}
void fp(auto& b,unsigned o,float v){put(b,o,std::bit_cast<unsigned>(v));}
int main(){
 // Real registered craft and ground peer; B500 fetches actual read-bank bytes.
 ArenaConstructionSimulationConfig config;config.capacity=4;config.native_extent=8;
 config.players={{{{0,0},0,false},100,100}};config.unit_base_available[0]=true;
 config.unit_quotas[0].capacity=8;config.unit_quotas[0].pending=config.unit_quotas[0].by_type[0]=1;
 ArenaState arena;arena.extent=8;arena.players.resize(1);arena.players[0].start_grid=ArenaGridPosition{0,0};
 NativeConstructionScene scene(arena,config);ArenaEntitySlot reused{};
 assert(scene.create_player_craft(0,{3.4f,0,3.5f,0},reused,[](const ArenaCraftAuxiliaryRequest&){return -1;})==ArenaCraftRegistration::Registered);
 const auto peer=scene.spawn_reserved_ground_unit(0,40,{3,3});assert(peer.entity_index==1);
 auto body=scene.simulation().storage().banks[0][0];body[0x33]=4;body[0x1c]=0;body[0x1d]=1;
 put(body,0x60,word(body,0x60)&~0x8000u);fp(body,0x40,10);
 ArenaEntityFinalizationContext commit;commit.position_480={3.4f,0,3.5f,0};commit.velocity_4a0={10,0,0,0};commit.previous_scalar_38_4e8=1000;
 assert(scene.finalize_player_craft(0,body,commit,1)==ArenaEntityFinalization::Committed);
 ArenaPreparationSnapshot snapshot{};put(snapshot,0x28d8,8);put(snapshot,0x28dc,8);put(snapshot,0x2790,0xffffffff);fp(snapshot,0x2560,.1f);fp(snapshot,0x2564,.05f);fp(snapshot,0x27b0,.016f);put(snapshot,0x27f8,0x100000);
 std::array<ArenaAccumulatorRecord,4> accumulators{};
 for(unsigned n=0;n<4;++n){put(accumulators[n],0,0x100000+128*n);put(accumulators[n],0x78,0xffffffff);}
 ArenaEntityWorkContext work{};ArenaPlayerReferenceCache cache;
 assert(scene.stage_player_craft_preparation(0,snapshot,work,cache,accumulators,9));
 assert(std::equal(work.begin()+0x100,work.begin()+0x200,scene.simulation().storage().banks[0][1].begin()));
 auto consumed=consume_arena_prepared_accumulator(work,{});assert(scene.complete_player_craft_preparation(0,snapshot,consumed,work)==ArenaPreparationCompletion::Prepared);
 ArenaPlayerControlScratch scratch;ArenaRandom random(71);ArenaParticleEmissionScratch particle_scratch;
 ArenaRequestPublicationQueue pool;pool.records.resize(1040);pool.free_ring.resize(1040);pool.available=1040;for(unsigned n=0;n<1040;++n)pool.free_ring[n]=n;
 unsigned particle_calls=0,impulse_writes=0;
 ArenaPlayerParticleTransport particles{&random,[&](const ArenaParticleEmission& q,ArenaRandom& rng){++particle_calls;assert(q.configuration[8]==48);assert(publish_arena_particles(pool,q,rng,particle_scratch).status==ArenaRequestPublicationStatus::Published);}};
 const auto saved=work;const auto saved_banks=scene.simulation().storage().banks;auto before_random=random;
 const auto missing=scene.advance_prepared_player_craft(0,snapshot,.016f,work,2,&scratch,{}, {},particles);
 assert(missing.control.status==ArenaPlayerControlStatus::NeedsPeerContact&&missing.control.blocked_address==0x16f8);
 assert(work==saved&&scene.simulation().storage().banks==saved_banks&&particle_calls==0);
 auto after_random=random;assert(before_random.next()==after_random.next());
 ArenaAccumulatorTransport transport{[&](unsigned ea)->std::optional<ArenaAccumulatorRecord>{assert(ea==0x100080);return accumulators[1];},[&](unsigned ea,const ArenaAccumulatorRecord& next){assert(ea==0x100080);++impulse_writes;accumulators[1]=next;return ArenaAccumulatorWrite::Committed;}};
 const auto step=scene.advance_prepared_player_craft(0,snapshot,.016f,work,2,&scratch,{}, {},particles,{},nullptr,transport);
 assert(step.control.status==ArenaPlayerControlStatus::Advanced&&step.finalization==ArenaEntityFinalization::Committed);
 assert(impulse_writes==1&&particle_calls==0&&pool.available==1040&&work[0xbd]==1);
 const auto& storage=scene.simulation().storage();assert(storage.banks[0]==storage.banks[1]);
 assert(word(accumulators[1],0x40)!=0);
 assert(scene.arena().entities[0].position.x==std::bit_cast<float>(word(storage.banks[0][0],0)));
 // Original1AD8 consumes the actual published impulse; the shared record is
 // cleared once while its returned snapshot retains the peer response.
 auto local=accumulators[1];const auto impulse=word(local,0x40);
 auto received=consume_arena_entity_accumulator(local,transport);
 assert(received.status==ArenaAccumulatorStatus::Consumed&&word(received.impulse_40,0)==impulse);
 assert(word(accumulators[1],0x40)==0&&impulse_writes==2);
 // Energetic side contact: source7510 publishes model48, count7 at5A0=6.
 // These input states are independently compared with the original instructions.
 auto energetic=saved;put(energetic,0x4f0,0);energetic[0x1c]=0;energetic[0x1d]=9;
 energetic[0x6c]=energetic[0x6d]=255;put(energetic,0x10c,0x00093228);
 energetic[0x11c]=energetic[0x11d]=255;
 fp(energetic,0x480,2.5f);fp(energetic,0x484,1);fp(energetic,0x488,3.5f);
 fp(energetic,0x100,2.6f);fp(energetic,0x104,1);fp(energetic,0x108,3.5f);
 fp(energetic,0x4a0,10);fp(energetic,0x4a4,0);fp(energetic,0x58,1);fp(energetic,0x158,2);
 fp(energetic,0x4e0,.2f);put(energetic,0x5a0,6);
 ArenaRandom impact_random(71);const auto impact=prepare_arena_player_peer_contact(energetic,snapshot,impact_random);
 assert(impact.advanced&&impact.overlapping&&impact.outgoing_impulse&&impact.particles);
 assert(impact.particles->count==7&&impact.particles->configuration[8]==48&&impact.particles->configuration[10]==1);
 // Ignored links and dead peer kinds take their source early returns without RNG.
 auto ignored=saved;ignored[0x6c]=ignored[0x1c];ignored[0x6d]=ignored[0x1d];auto rng_before=impact_random;
 assert(prepare_arena_player_peer_contact(ignored,snapshot,impact_random).advanced);
 assert(impact_random.next()==rng_before.next());
 ignored=saved;ignored[0x10f]=63;
 assert(prepare_arena_player_peer_contact(ignored,snapshot,impact_random).advanced&&ignored[0x1c]==255&&ignored[0x1d]==255);
 // Fixed16F8 contact writer must rebase on a fresh reservation after failure.
 ArenaAccumulatorRecord fresh{};fp(fresh,0x40,2);unsigned reads=0,writes=0;
 ArenaAccumulatorTransport retry{[&](unsigned)->std::optional<ArenaAccumulatorRecord>{++reads;auto b=fresh;if(reads==2)fp(b,0x40,7);return b;},[&](unsigned,const ArenaAccumulatorRecord& b){++writes;assert(word(b,0x40)==std::bit_cast<unsigned>(writes==1?3.f:8.f));return writes==1?ArenaAccumulatorWrite::Retry:ArenaAccumulatorWrite::Committed;}};
 assert(submit_arena_peer_impulse(local,0x100080,{1,0,0,0},retry)==ArenaAccumulatorStatus::Cached);
 assert(reads==2&&writes==2&&word(local,0x40)==std::bit_cast<unsigned>(8.f));
 const auto no_transport=local;assert(submit_arena_peer_impulse(local,0,{1,0,0,0},{})==ArenaAccumulatorStatus::MissingTransport&&local==no_transport);
}
