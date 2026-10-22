#include "comet/arena_player_control.hpp"
#include "comet/arena_preparation_effects.hpp"
#include "comet/arena_request_publication.hpp"
#include "comet/native_construction_scene.hpp"
#include <algorithm>
#include <bit>
#include <cassert>
using namespace comet::decomp;
namespace {
unsigned word(const auto& b,unsigned o){return unsigned(b[o])<<24|unsigned(b[o+1])<<16|unsigned(b[o+2])<<8|b[o+3];}
void put(auto& b,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(24-8*i));}
void fp(auto& b,unsigned o,float v){put(b,o,std::bit_cast<unsigned>(v));}
float scalar(const auto& b,unsigned o){return std::bit_cast<float>(word(b,o));}
ArenaPreparationSnapshot snapshot(){ArenaPreparationSnapshot s{};put(s,0x28d8,8);put(s,0x28dc,8);put(s,0x2790,0xffffffff);fp(s,0x2560,.1f);fp(s,0x2564,.05f);return s;}
ArenaEntityWorkContext prepared(const ArenaPreparationSnapshot& s){
 ArenaEntitySlot body{};initialize_arena_player_craft_entity(body,0,2.5f,3.5f);
 ArenaEntityWorkContext c{};std::copy(body.begin(),body.end(),c.begin());
 assert(stage_arena_entity_preparation(c,s));auto effects=consume_arena_prepared_accumulator(c,{});
 std::array<std::uint8_t,128> stats{};
 assert(complete_arena_entity_preparation(c,s,effects,stats)==ArenaPreparationCompletion::Prepared);return c;
}
}
int main(){
 auto s=snapshot();auto c=prepared(s),initial=c;ArenaPlayerControlScratch scratch;
 scratch.pending_entity.fill(0x5a);scratch.owner_effect.fill(0x6b);scratch.action_effect.fill(0xa5);
 const auto original_scratch=scratch;unsigned reserves=0,publishes=0;
 ArenaPlayerPendingTransport pending;
 pending.reserve=[&](const ArenaEntitySlot& entity){++reserves;assert(entity[15]==7&&entity[0x6f]==33&&entity[0x83]==1);assert(scalar(entity,0)==0&&scalar(entity,4)==0&&scalar(entity,8)==0);assert(entity[0x90]==0x5a);return 9;};
 pending.publish=[&](const ArenaEntitySlot& entity){++publishes;assert(entity[12]==0&&entity[13]==9);};
 assert(advance_arena_player_control(c,s,.016f,&scratch).status==ArenaPlayerControlStatus::NeedsPendingQueue&&c==initial);
 assert(advance_arena_player_control(c,s,.016f,&scratch,pending).status==ArenaPlayerControlStatus::Advanced);
 assert(reserves==1&&publishes==1&&!(word(c,0x60)&0x8000));
 assert(scalar(c,0x480)==2.5f&&scalar(c,0x484)==0&&scalar(c,0xe4)==1);
 assert(scalar(c,0x4a0)==4&&scalar(c,0x4a8)==2&&scalar(c,0x4a4)==0);
 assert(c[0x33]==0&&scalar(c,0x78)==std::bit_cast<float>(0x3c23d70au));
 assert(word(c,0x20)==word(initial,0xf0)&&word(c,0x24)==word(initial,0xf4));
 // Rejection is transactional, including external callbacks and reused bytes.
 auto reject=[&](ArenaEntityWorkContext work,ArenaPreparationSnapshot image,ArenaPlayerControlStatus expected){
  const auto before=work;auto sc=original_scratch;const auto calls=reserves;
  const auto result=advance_arena_player_control(work,image,.016f,&sc,pending);
  assert(result.status==expected&&work==before&&sc.pending_entity==original_scratch.pending_entity&&sc.owner_effect==original_scratch.owner_effect&&sc.action_effect==original_scratch.action_effect&&reserves==calls);
 };
 auto work=initial;put(s,0x28d4,1);reject(work,s,ArenaPlayerControlStatus::NeedsBanking);s=snapshot();
 work[0xb8]=work[0xb9]=0;reject(work,s,ArenaPlayerControlStatus::NeedsAttachment);work=initial;
 work[0xbf]=1;fp(work,0x200,4);fp(work,0x204,1);fp(work,0x208,1);
 assert(advance_arena_player_control(work,s,.016f,&scratch,pending).status==ArenaPlayerControlStatus::Advanced);work=initial;
 work[0x1c]=work[0x1d]=0;reject(work,s,ArenaPlayerControlStatus::NeedsPeerContact);work=initial;
 fp(s,0x2560,0);fp(s,0x2564,1);reject(work,s,ArenaPlayerControlStatus::NeedsNumericRecovery);s=snapshot();
 // Source owner timer crosses .5 once, preserving unwritten request metadata.
 work=initial;put(work,0x60,word(work,0x60)&~0x8000u);work[0xc4]=0;work[0xc5]=1;fp(work,0xb4,.49f);put(s,0x2790,1);
 reject(work,s,ArenaPlayerControlStatus::NeedsOwnerEmission);
 ArenaRequestPublicationQueue queue;queue.records.resize(160);queue.free_ring.resize(160);queue.available=160;
 for(unsigned i=0;i<160;++i)queue.free_ring[i]=i;
 auto emit=[&](const ArenaCounterEmissionRequest& request){assert(request.descriptor[15]==109&&word(request.descriptor,0x10)==16);assert(publish_arena_request(queue,request).status==ArenaRequestPublicationStatus::Published);};
 assert(advance_arena_player_control(work,s,.016f,&scratch,{},emit).status==ArenaPlayerControlStatus::Advanced);
 assert(queue.available==159&&scalar(work,0xb4)>.5f&&queue.records[0][0x2c]==0x6b&&queue.records[0][0x2d]==0x6b);
 // Held action now executes original release requests and no-target fallback.
 s=snapshot();fp(s,0x2584,1);work=initial;work[0xbc]=0;work[0xbb]=1;
 put(work,0x378,0xffffffff);work[0x1e]=work[0x1f]=0xff;
 reject(work,s,ArenaPlayerControlStatus::NeedsActionEmission);
 unsigned action_emits=0;
 auto action_emit=[&](const ArenaCounterEmissionRequest& r){
  const auto index=action_emits++%4;
  assert(word(r.descriptor,0x10)==18&&r.descriptor[15]==112);
  assert(scalar(r.descriptor,0)==-100&&scalar(r.descriptor,4)==-100&&scalar(r.descriptor,8)==-100);
  assert(scalar(r.descriptor,0x30)==(index<2?1.f:-1.f));
  assert(r.descriptor[0x41]==index%2&&r.descriptor[0x43]==21&&r.descriptor[0x48]==1);
  assert(r.descriptor[0x50]==0xa5); // untouched reused byte

  assert(r.count==1&&r.mode==0);
 };
 assert(advance_arena_player_control(work,s,.016f,&scratch,pending,action_emit).status==ArenaPlayerControlStatus::Advanced);
 assert(action_emits==4&&work[0xbb]==0&&work[0xbe]==0&&work[0xbf]==0);
 assert(work[0x1e]==0xff&&work[0x1f]==0xff);
 // Keeping the command held does not emit release requests again.
 assert(advance_arena_player_control(work,s,.016f,&scratch,pending,action_emit).status==ArenaPlayerControlStatus::Advanced);
 assert(action_emits==4);
 // Release resets the edge latch, so a fresh press emits four again.
 fp(s,0x2584,0);
 assert(advance_arena_player_control(work,s,.016f,&scratch,pending,action_emit).status==ArenaPlayerControlStatus::Advanced&&work[0xbb]==1);
 fp(s,0x2584,1);
 assert(advance_arena_player_control(work,s,.016f,&scratch,pending,action_emit).status==ArenaPlayerControlStatus::Advanced&&action_emits==8);
 // Pickup candidate stays selected and enters target facing; release clears it.
 work=initial;work[0xbc]=work[0xbb]=work[0xbe]=0;work[0x1e]=0;work[0x1f]=3;
 put(work,0x378,0xffffffff);fp(work,0x200,2.75f);fp(work,0x204,1);fp(work,0x208,2.75f);work[0x20f]=40;
 fp(s,0x2560,0);fp(s,0x2564,0);
 assert(advance_arena_player_control(work,s,.016f,&scratch,pending,action_emit).status==ArenaPlayerControlStatus::Advanced);
 assert(work[0xbe]==1&&work[0xbf]==1&&work[0x1f]==3&&work[0xbb]==0);
 assert(advance_arena_player_control(work,s,.016f,&scratch,pending,action_emit).status==ArenaPlayerControlStatus::Advanced);
 assert(work[0xbf]==1); // target-facing continuation is executable
 fp(s,0x2584,0);
 assert(advance_arena_player_control(work,s,.016f,&scratch,pending,action_emit).status==ArenaPlayerControlStatus::Advanced);
 assert(work[0xbe]==0&&work[0xbf]==0&&work[0xbb]==1&&work[0x1f]==255);
 // An ineligible candidate uses the original fallback without pretending pickup.
 work=initial;work[0xbc]=work[0xbb]=0;work[0x1e]=0;work[0x1f]=3;
 fp(work,0x200,2.75f);fp(work,0x204,1);fp(work,0x208,2.75f);work[0x20f]=61;
 put(work,0x378,17);fp(s,0x2584,1);
 assert(advance_arena_player_control(work,s,.016f,&scratch,pending,action_emit).status==ArenaPlayerControlStatus::Advanced);
 assert(work[0xbe]==0&&work[0xbf]==0&&work[0x1e]==0&&work[0x1f]==17);
 // Original creation ->9A70 ->10290 ->6578 ->authoritative rendered scene.
 // A one-slot pool is actually full, so original pending allocation must fail;
 // the source retains8000 and continues movement (no invented success).
 ArenaState arena;arena.extent=8;arena.players.resize(1);arena.players[0].start_grid=ArenaGridPosition{0,0};
 for(auto& cell:arena.cells)cell.visibility_recovered=true;
 ArenaConstructionSimulationConfig config;config.capacity=1;config.native_extent=8;config.players={{{{0,0},0,false},100,100}};
 NativeConstructionScene scene(arena,config);ArenaEntitySlot reused{};
 assert(scene.create_player_craft(0,{2.5f,0,3.5f,0},reused,[](const ArenaCraftAuxiliaryRequest&){return -1;})==ArenaCraftRegistration::Registered);
 assert(scene.arena().entities.size()==1&&scene.arena().entities[0].live_entity_index==0);
 ArenaPlayerPendingTransport full;
 unsigned full_reserves=0;
 full.reserve=[&](const ArenaEntitySlot&){++full_reserves;assert(scene.simulation().storage().free_indices.empty());return -1;};
 full.publish=[](const ArenaEntitySlot&){assert(false);};s=snapshot();ArenaEntityWorkContext context{};
 for(unsigned frame=1;frame<=4;++frame){
  assert(scene.stage_player_craft_preparation(0,s,context));auto effects=consume_arena_prepared_accumulator(context,{});
  assert(scene.complete_player_craft_preparation(0,s,effects,context)==ArenaPreparationCompletion::Prepared);
  auto result=scene.advance_prepared_player_craft(0,s,.016f,context,frame,&scratch,full);
  assert(result.control.status==ArenaPlayerControlStatus::Advanced&&result.finalization==ArenaEntityFinalization::Committed);
  const auto& storage=scene.simulation().storage();assert(storage.banks[0]==storage.banks[1]);
  const auto& entity=scene.arena().entities[0];const auto& body=storage.banks[0][0];
  assert(entity.position.x==scalar(body,0)&&entity.position.y==scalar(body,4)&&entity.position.z==scalar(body,8));
  assert(word(body,0x60)&0x8000);if(frame<=2)assert(entity.position.x==2.5f);else assert(entity.position.x>2.5f);
 }
 const auto banks=scene.simulation().storage().banks;
 assert(scene.advance_prepared_player_craft(1,s,.016f,context,5,&scratch,full).control.status==ArenaPlayerControlStatus::Invalid);
 assert(scene.simulation().storage().banks==banks&&scene.arena().entities.size()==1);
 // A blocked finalization resumes through the scene without rerunning10290.
 assert(scene.stage_player_craft_preparation(0,s,context));
 auto effects=consume_arena_prepared_accumulator(context,{});
 assert(scene.complete_player_craft_preparation(0,s,effects,context)==ArenaPreparationCompletion::Prepared);
 fp(context,0x38,99); // actual health change requires still-unrecovered3940
 auto blocked=scene.advance_prepared_player_craft(0,s,.016f,context,5,&scratch,full);
 assert(blocked.control.status==ArenaPlayerControlStatus::NeedsFinalization);
 assert(blocked.finalization==ArenaEntityFinalization::NeedsChangeEffects);
 const auto calls=full_reserves;ArenaEntitySlot retained;
 std::copy_n(context.begin(),256,retained.begin());
 assert(scene.finalize_player_craft(0,retained,blocked.commit,5)==ArenaEntityFinalization::NeedsChangeEffects);
 assert(full_reserves==calls&&scene.simulation().storage().banks==banks);

 // Connected press/hold/release: actual storage retains the source edge latch;
 // the recovered1E090 publisher receives all ordered kind18 descriptors.
 // BC0 is an explicitly supplied original controller state, not a new bootstrap.
 ArenaRequestPublicationQueue action_queue;action_queue.records.resize(160);
 action_queue.free_ring.resize(160);action_queue.available=160;
 for(unsigned i=0;i<160;++i)action_queue.free_ring[i]=i;
 auto publish_action=[&](const ArenaCounterEmissionRequest& request){
  assert(publish_arena_request(action_queue,request).status==ArenaRequestPublicationStatus::Published);
 };
 for(unsigned step=0;step<4;++step){
  assert(scene.stage_player_craft_preparation(0,s,context));
  auto consumed=consume_arena_prepared_accumulator(context,{});
  assert(scene.complete_player_craft_preparation(0,s,consumed,context)==ArenaPreparationCompletion::Prepared);
  context[0xbc]=0;put(context,0x378,0xffffffff); // no fallback target
  fp(s,0x2584,step==2?0.f:1.f);
  auto action=scene.advance_prepared_player_craft(0,s,.016f,context,6+step,&scratch,full,publish_action);
  assert(action.control.status==ArenaPlayerControlStatus::Advanced);
  assert(action.finalization==ArenaEntityFinalization::Committed);
  assert(scene.simulation().storage().banks[0]==scene.simulation().storage().banks[1]);
  assert(action_queue.available==(step==3?152u:156u));
 }
 for(unsigned i=0;i<8;++i)assert(word(action_queue.records[i],0x10)==18&&action_queue.records[i][15]==112);
}
