#include "comet/arena_ground_carry.hpp"
#include "comet/arena_unit_entity.hpp"
#include "comet/native_construction_scene.hpp"
#include <algorithm>
#include <bit>
#include <cassert>
#include <utility>
#include <limits>
using namespace comet::decomp;
namespace {
unsigned word(const auto& b,unsigned o){return unsigned(b[o])<<24|unsigned(b[o+1])<<16|unsigned(b[o+2])<<8|b[o+3];}
void put(auto& b,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(24-8*i));}
void fp(auto& b,unsigned o,float v){put(b,o,std::bit_cast<unsigned>(v));}
float scalar(const auto& b,unsigned o){return std::bit_cast<float>(word(b,o));}
ArenaGroundCarryContext fixture(unsigned parentkind,unsigned kind=40){
 ArenaGroundCarryContext work{};ArenaEntitySlot body{};ArenaRandom random(5);
 initialize_arena_unit_template(body,kind,0,{1,0,1,0},random);
 std::copy(body.begin(),body.end(),work.begin());put(work,12,0x00013200|kind);
 put(work,0xb4,0x00020003);put(work,0x500,kind);put(work,0x510,5);work[0x595]=1;
 fp(work,0x18,.3f);put(work,0x60,word(work,0x60)|0x800u);
 put(work,0x520,64);put(work,0x530,2);put(work,0x5d0,0x80);
 fp(work,0x480,1);fp(work,0x484,1);fp(work,0x488,1);
 fp(work,0x4a0,2);fp(work,0x4a4,.1f);fp(work,0x4a8,.3f);fp(work,0xc4,1);
 fp(work,0xa00,3);fp(work,0xa04,1.5f);fp(work,0xa08,3);
 put(work,0xa0c,0x00023200|parentkind);work[0xa33]=0x38;work[0xa7e]=3;
 fp(work,0xa20,.1f);fp(work,0xa24,.2f);fp(work,0xa28,.3f);fp(work,0xa2c,.9273618f);
 return work;
}
ArenaPreparationSnapshot snapshot(){ArenaPreparationSnapshot s{};s[0x2723]=2;put(s,0x28e8,0xffffffff);put(s,0x28d8,8);put(s,0x28dc,8);return s;}
void patch(auto& b,std::initializer_list<std::pair<unsigned,unsigned>> changes){for(auto [o,v]:changes)put(b,o,v);}
}
int main(){
 // Original FE5D8 interpreter outputs: full C00 context comparison. Inputs
 // are synthetic source states; these are not a recovered pickup producer.
 auto image=snapshot();auto work=fixture(1),expected=work;
 patch(expected,{{0x7c,0x0a8c0300},{0x480,0x3f81cf6e},{0x484,0x3f8091ec},{0x488,0x3f826a65},{0x48c,0x39af5e4b}});
 assert(prepare_arena_ground_carry_position(work,image,.016f)==ArenaGroundCarryStatus::Advanced&&work==expected);
 work=fixture(1);expected=work;
 patch(expected,{{0x10,124},{0x18,0x3f147ae1},{0x20,0x3bc1e43b},{0x24,0x3d20a54e},{0x28,0xb93b9be5},{0x2c,0x3f7fcc6d},{0x30,0x00000c19},{0x60,0x6006},{0xbc,3},{0xc4,0x3f7be76c},{0x4a0,0x3efccb6d},{0x4a4,0x3cde4ac9},{0x4a8,0x3d97ad41}});
 assert(advance_arena_ground_carry_dynamics(work,image,.016f)==ArenaGroundCarryStatus::Advanced&&work==expected);
 // Kind43 exposes the source's different X/Z and Y/W norm rounding.
 work=fixture(1,43);fp(work,0x4a0,.2f);
 assert(advance_arena_ground_carry_dynamics(work,image,.016f)==ArenaGroundCarryStatus::Advanced);
 assert(word(work,0x24)==0x3d206843&&word(work,0x2c)==0x3f7fcc5c);
 work=fixture(62);expected=work;
 patch(expected,{{0x10,124},{0x18,0x3f147ae1},{0x60,0x6006},{0x8c,0x40000000},{0xb4,0xffffffff},{0xb8,0x01000000},{0xbc,3},{0xc4,0x3f7be76c}});
 assert(advance_arena_ground_carry_dynamics(work,image,.016f)==ArenaGroundCarryStatus::Advanced&&work==expected);
 // Fetch actual bank references, preserve absent-cache bytes, reject bad IDs.
 std::array<ArenaEntitySlot,4> bank{};std::array<ArenaAccumulatorRecord,4> accum{};
 for(unsigned n=0;n<4;++n){bank[n].fill(std::uint8_t(10+n));accum[n].fill(std::uint8_t(20+n));}
 work=fixture(1);assert(fetch_arena_ground_carry_references(work,bank,accum,7));
 assert(std::equal(bank[2].begin(),bank[2].end(),work.begin()+0xa00));
 assert(std::equal(bank[3].begin(),bank[3].end(),work.begin()+0xb00));
 assert(work[0x300]==21&&word(work,0x59c)==7);
 put(work,0xb4,0xffffffff);const auto retained=work;
 assert(fetch_arena_ground_carry_references(work,bank,accum,7)&&work==retained);
 put(work,0xb4,0x80000003);const auto invalid=work;
 assert(!fetch_arena_ground_carry_references(work,bank,accum,7)&&work==invalid);
 //6568/6260 source fixture: packed category2 is selected, and the full
 // SIMD score cache retains100 in lanes masked out by the source body.
 ArenaGroundCarryContext spatial{};put(spatial,0x10,124);put(spatial,0x4f0,1);fp(spatial,0x4e0,.5f);
 fp(spatial,0x480,1);fp(spatial,0x484,1);fp(spatial,0x488,1);
 const std::array<ArenaPackedCellEntry,1> peers{{{0,2,50,40,100,100,100,8}}};
 ArenaSpatialNeighborhood neighborhood;neighborhood.cell_x=neighborhood.cell_z=1;neighborhood.cells[4]=peers;
 auto spatial_expected=spatial;patch(spatial_expected,{{0x1c,0x00020000},{0x5b0,0xbe0930ab},{0x5b4,0x42c80000},{0x5b8,0x42c80000},{0x5bc,0x42c80000}});
 assert(select_arena_entity_peer(spatial,neighborhood)&&spatial==spatial_expected);
 // Registered parent + spawned ground unit -> supplied14060 output -> actual
 // banks, ordered cell migration and scene projection. No preview advance.
 for(auto& record:accum)record.fill(0);
 ArenaState arena;arena.extent=8;arena.players.resize(1);arena.players[0].start_grid=ArenaGridPosition{0,0};
 for(auto& cell:arena.cells)cell.visibility_recovered=true;
 ArenaConstructionSimulationConfig config;config.capacity=4;config.native_extent=8;
 config.players={{{{0,0},0,false},100,100}};
 config.unit_quotas[0].capacity=1;config.unit_quotas[0].pending=config.unit_quotas[0].by_type[0]=1;
 NativeConstructionScene scene(arena,config);ArenaEntitySlot reused{};
 assert(scene.create_player_craft(0,{3,0,3,0},reused,[](const ArenaCraftAuxiliaryRequest&){return -1;})==ArenaCraftRegistration::Registered);
 assert(scene.spawn_reserved_ground_unit(0,40,{1,1}).entity_index==1);
 auto parent=scene.simulation().storage().banks[0][0];
 fp(parent,0x20,.1f);fp(parent,0x24,.2f);fp(parent,0x28,.3f);fp(parent,0x2c,.9273618f);
 ArenaEntityFinalizationContext pc;pc.position_480={3,1.5f,3,0};pc.previous_scalar_38_4e8=scalar(parent,0x38);
 assert(scene.finalize_player_craft(0,parent,pc,1)==ArenaEntityFinalization::Committed);
 work=fixture(1);assert(scene.stage_ground_carry_references(1,work,accum,9));
 std::copy_n(parent.begin(),256,work.begin()+0xa00);
 fp(work,0x4e8,scalar(work,0x38));fp(work,0x4e4,scalar(work,0x38));fp(work,0x4e0,.5f);put(work,0x4f0,1);
 // Source command2 + successful14F8 claim creates the link from an
 // unlinked spawned record. The request producer remains an explicit input.
 assert(word(work,0xb4)==0xffffffff);
 put(work,0x360,2);put(work,0x364,0x0000ffff);put(work,0x368,0x00070000);put(work,0x36c,0x8001ffff);
 put(work,0x60,word(work,0x60)|0x2000);put(image,0x27f8,0x80000000);
 ArenaAccumulatorRecord command_scratch{},shared{};unsigned command_reads=0,command_writes=0;
 ArenaAccumulatorTransport command_transport;
 command_transport.read_reserved=[&](unsigned address)->std::optional<ArenaAccumulatorRecord>{assert(address==0x80000000);++command_reads;return shared;};
 command_transport.write_conditional=[&](unsigned address,const ArenaAccumulatorRecord& record){assert(address==0x80000000);++command_writes;shared=record;return ArenaAccumulatorWrite::Committed;};
 ArenaCommandClaimResult claim;
 assert(scene.complete_prepared_ground_carry_command(1,image,.016f,work,1,{1,1},command_scratch,command_transport,&claim)==ArenaGroundCarryStatus::Advanced);
 assert(claim.status==ArenaCommandClaimStatus::Claimed&&claim.attempts==1&&word(shared,0x60)==3);
 assert(word(shared,0x64)==0x0000ffff&&word(shared,0x68)==0x00070000);
 assert(work[0xb4]==0&&work[0xb5]==0&&work[0xb2]==0&&work[0xb3]==7&&!(word(work,0x60)&0x2000));
 assert(scene.simulation().storage().banks[0][1]==scene.simulation().storage().banks[1][1]);
 assert(scene.stage_ground_carry_references(1,work,accum,10)&&work[0xa0f]==1);
 for(unsigned frame=2;frame<5;++frame){
  fp(work,0x480,float(frame)+.25f);fp(work,0x484,1);fp(work,0x488,2.25f);
  assert(scene.advance_prepared_ground_carry(1,image,.016f,work,frame,{std::uint8_t(frame==2?1:frame-1),std::uint8_t(frame==2?1:2)})==ArenaGroundCarryStatus::Advanced);
  const auto& storage=scene.simulation().storage();assert(storage.banks[0]==storage.banks[1]);
  const auto& body=storage.banks[0][1];assert(word(body,0)==word(work,0x480)&&word(body,0x40)==word(work,0x4a0));
  assert(storage.cells[0][2*24+frame].size()==1);
  const auto& rendered=scene.arena().entities[1];assert(rendered.position.x==scalar(body,0)&&rendered.orientation[1]==scalar(body,0x24));
 }
 // Actual parent tombstone is fetched through12CD0, then original17EE8 clears
 // links and restores the falling stage in both banks and rendered state.
 parent=scene.simulation().storage().banks[0][0];pc.removal_590=1;
 assert(scene.finalize_player_craft(0,parent,pc,5)==ArenaEntityFinalization::Tombstone);
 assert(scene.stage_ground_carry_references(1,work,accum,10)&&work[0xa0f]==63);
 assert(scene.advance_prepared_ground_carry(1,image,.016f,work,5,{4,2})==ArenaGroundCarryStatus::Advanced);
 const auto& lost=scene.simulation().storage().banks[0][1];
 assert(word(lost,0xb4)==0xffffffff&&lost[0xb8]==1&&word(lost,0x8c)==0x40000000&&(lost[0x33]&28)==20);
 // A successful externally published claim followed by an unrecovered
 // base event retains controller output. Explicit finalization resumes it
 // without reissuing the claim, even though bank admission was deferred.
 work=fixture(1);std::copy_n(lost.begin(),256,work.begin());put(work,0xb4,0xffffffff);fp(work,0x8c,0);
 fp(work,0x480,2.5f);fp(work,0x484,.1f);fp(work,0x488,2.5f);work[0x591]=0;
 fp(work,0x4e8,scalar(work,0x38));fp(work,0x4e4,scalar(work,0x38));fp(work,0x4e0,.5f);put(work,0x4f0,1);
 put(work,0x360,2);put(work,0x364,0x0000ffff);put(work,0x368,0x00090000);put(work,0x60,word(work,0x60)|0x2000);
 image[0x25b8]=1;put(image,0x2774,0);put(image,0x2664,2);put(image,0x2674,2);put(shared,0x60,0);
 const auto before=scene.simulation().storage();const auto writes=command_writes;
 assert(scene.complete_prepared_ground_carry_command(1,image,.016f,work,6,{4,2},command_scratch,command_transport,&claim)==ArenaGroundCarryStatus::NeedsFinalization);
 assert(claim.status==ArenaCommandClaimStatus::Claimed&&word(work,0x360)==3&&word(work,0xb4)==0x0000ffff);
 assert(command_writes==writes+1&&scene.simulation().storage().banks==before.banks&&scene.simulation().storage().cells==before.cells);
 image[0x28e1]=1;
 assert(scene.finalize_prepared_ground_carry(1,image,work,6,{4,2})==ArenaGroundCarryStatus::Advanced);
 assert(command_writes==writes+1&&word(scene.simulation().storage().banks[0][1],0xb4)==0x0000ffff);
 // Occupied publication still changes360 to3, while preserving the old link.
 work=fixture(1);put(work,0xb4,0xffffffff);put(work,0x360,2);put(work,0x364,0x0000ffff);put(work,0x60,word(work,0x60)|0x2000);
 assert(complete_arena_ground_carry_command(work,image,.016f,command_scratch,command_transport,&claim)==ArenaGroundCarryStatus::Advanced);
 assert(claim.status==ArenaCommandClaimStatus::Occupied&&word(work,0xb4)==0xffffffff&&word(work,0x360)==3&&(word(work,0x60)&0x2000));
 // Actual kind47 carrier -> peer command2 ->1AD8 consumption -> B4 link ->
 // parent command3 ->1AD8 consumption -> original audio60 acknowledgement.
 // Only the preceding14060/ordinary158B8 motion/contact outputs are supplied.
 ArenaConstructionSimulationConfig cc;cc.capacity=4;cc.native_extent=8;
 cc.players={{{{0,0},0,false},100,100}};
 cc.unit_quotas[0].capacity=2;cc.unit_quotas[0].pending=2;
 cc.unit_quotas[0].by_type[7]=cc.unit_quotas[0].by_type[0]=1;
 NativeConstructionScene carrier_scene(arena,cc);
 assert(carrier_scene.spawn_reserved_ground_unit(0,47,{3,3}).entity_index==0);
 assert(carrier_scene.spawn_reserved_ground_unit(0,40,{3,3}).entity_index==1);
 auto cs=snapshot();put(cs,0x27f8,0x81000000);
 std::array<ArenaAccumulatorRecord,2> records{};
 for(unsigned n=0;n<2;++n){put(records[n],0,0x81000000+128*n);put(records[n],0x78,0xffffffff);}
 ArenaAccumulatorTransport channel;
 channel.read_reserved=[&](unsigned ea)->std::optional<ArenaAccumulatorRecord>{assert(ea>=0x81000000&&ea<0x81000100&&(ea&127)==0);return records[(ea-0x81000000)/128];};
 channel.write_conditional=[&](unsigned ea,const ArenaAccumulatorRecord& r){records[(ea-0x81000000)/128]=r;return ArenaAccumulatorWrite::Committed;};
 auto stage=[&](unsigned index){
  ArenaGroundCarryContext w{};assert(carrier_scene.stage_ground_carry_references(std::uint16_t(index),w,records,15));
  put(w,0x500,w[15]);put(w,0x510,5);put(w,0x5d0,0x80);put(w,0x4f0,index);w[0x595]=1;
  fp(w,0x4e0,float(w[14])*.01f);fp(w,0x4e4,scalar(w,0x38));fp(w,0x4e8,scalar(w,0x38));
  fp(w,0x480,3.125f+float(index)*.125f);fp(w,0x484,.75f);fp(w,0x488,3.125f);return w;
 };
 auto carrier=stage(0);assert((word(carrier,0x60)&0x1000)&&carrier[0xb1]==20);
 std::copy(carrier_scene.simulation().storage().banks[0][1].begin(),carrier_scene.simulation().storage().banks[0][1].end(),carrier.begin()+0x100);
 fp(carrier,0x570,.25f);fp(carrier,0x20,.1f);fp(carrier,0x24,.2f);fp(carrier,0x28,.3f);fp(carrier,0x2c,.9273618f);
 ArenaAccumulatorRecord stack{};ArenaCommandClaimResult transfer;
 const auto unclaimed=records;const auto before_claim=carrier;
 assert(carrier_scene.complete_prepared_ground_carry_command(0,cs,std::numeric_limits<float>::quiet_NaN(),carrier,1,{3,3},stack,channel,&transfer)==ArenaGroundCarryStatus::Invalid);
 assert(records==unclaimed&&carrier==before_claim);
 assert(carrier_scene.complete_prepared_ground_carry_command(0,cs,.016f,carrier,1,{3,3},stack,channel,&transfer)==ArenaGroundCarryStatus::Advanced);
 assert(transfer.status==ArenaCommandClaimStatus::Claimed&&word(records[1],0x60)==2);
 assert(word(records[1],0x64)==0x00000001&&word(records[1],0x68)==0x00000001&&word(records[1],0x6c)==0x00001000);
 assert(carrier[0xb1]==19&&(word(carrier,0x60)&0x200000));
 auto passenger=stage(1);
 auto consume=[&](ArenaGroundCarryContext& w){
  ArenaEntityWorkContext prefix;std::copy_n(w.begin(),prefix.size(),prefix.begin());
  const auto consumed=consume_arena_prepared_accumulator(prefix,channel);
  assert(consumed.status==ArenaAccumulatorStatus::Consumed);
  std::copy(prefix.begin(),prefix.end(),w.begin());
 };
 consume(passenger);assert(word(passenger,0x360)==2&&word(records[1],0x60)==0);
 assert(carrier_scene.complete_prepared_ground_carry_command(1,cs,.016f,passenger,2,{3,3},stack,channel,&transfer)==ArenaGroundCarryStatus::Advanced);
 assert(word(passenger,0xb4)==0x0000ffff&&passenger[0xb3]==0&&word(records[0],0x60)==3);
 assert(carrier_scene.stage_ground_carry_references(1,passenger,records,16)&&passenger[0xa0f]==47);
 // The next update consumes the cleared own accumulator; stale local opcode3
 // must not be replayed as a new acknowledgement on the passenger.
 assert(word(passenger,0x360)==0);
 assert(prepare_arena_ground_carry_position(passenger,cs,.016f)==ArenaGroundCarryStatus::Advanced);
 assert(carrier_scene.advance_prepared_ground_carry(1,cs,.016f,passenger,3,{3,3})==ArenaGroundCarryStatus::Advanced);
 carrier=stage(0);consume(carrier);assert(word(carrier,0x360)==3&&word(records[0],0x60)==0);
 unsigned acknowledgements=0;
 assert(carrier_scene.complete_prepared_ground_carry_command(0,cs,.016f,carrier,4,{3,3},stack,channel,&transfer,nullptr,[&](const ArenaAudioEvent& e){assert(e.sound==60&&e.parameter_6==3&&e.parameter_7==3);++acknowledgements;})==ArenaGroundCarryStatus::Advanced);
 assert(acknowledgements==1&&!(word(carrier,0x60)&0x200000)&&carrier[0xb1]==19);
 assert(carrier_scene.simulation().storage().banks[0]==carrier_scene.simulation().storage().banks[1]);
 assert(carrier_scene.arena().entities[1].position.x==scalar(carrier_scene.simulation().storage().banks[0][1],0));

}
