#include "comet/arena_entity_finalization.hpp"
#include "comet/arena_entity_core.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
std::uint32_t word(const ArenaEntitySlot& s,unsigned o){
 return std::uint32_t(s[o])<<24|std::uint32_t(s[o+1])<<16|std::uint32_t(s[o+2])<<8|s[o+3];
}
void put(ArenaEntitySlot& s,unsigned o,std::uint32_t v){for(unsigned i=0;i<4;++i)s[o+i]=std::uint8_t(v>>(24-8*i));}
bool normal(float v){return std::isfinite(v) && std::fpclassify(v)!=FP_SUBNORMAL;}
struct Rounding {
 int previous=std::fegetround();
 Rounding(){if(previous<0 || std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("entity finalization rounding unavailable");}
 ~Rounding(){std::fesetround(previous);}
};
struct Membership {unsigned flat=0;std::array<unsigned,2> offset{};};
std::optional<Membership> membership(const ArenaEntityStorage& s,std::uint16_t index){
 if(s.read_bank>1 || !s.width || !s.height || s.width>24 || s.height>24 ||
    index>=s.banks[0].size() || index>=s.banks[1].size() ||
    s.banks[0][index]!=s.banks[1][index] || (word(s.banks[0][index],12)>>16)!=index ||
    std::find(s.free_indices.begin(),s.free_indices.end(),index)!=s.free_indices.end())return {};
 Membership m;std::array<unsigned,2> count{},flat{};
 for(unsigned b=0;b<2;++b)for(unsigned f=0;f<576;++f)
  for(unsigned n=0;n<s.cells[b][f].size();++n){
   const auto& e=s.cells[b][f][n];
   if((unsigned(e[0])<<8|e[1])==index){++count[b];flat[b]=f;m.offset[b]=n;}
  }
 if(count[0]!=1 || count[1]!=1 || flat[0]!=flat[1] || flat[0]%24>=s.width ||
    flat[0]/24>=s.height || !s.summary_counts[flat[0]])return {};
 m.flat=flat[0];return m;
}
}
void initialize_arena_entity_tombstone(ArenaEntitySlot& slot,std::uint32_t frame){
 // 3400 fixed arguments to2DF8. For this fixed zero-quantity constructor the
 // shared PPU FECFC field writes are identical: XYZ/quaternion, common flags,
 // scalars, markers and sentinel halfwords. The second half is not cleared.
 ArenaEntityCoreParameters p;p.type=63;p.model_slot=40;
 p.position={-10,-10,0,0};p.scalar_58=1;p.scalar_38=p.scalar_64=100;
 initialize_arena_entity_core(std::span<std::uint8_t,256>(slot).first<128>(),p);
 put(slot,0x80,frame+3u);
}
bool arena_entity_tombstone_due(const ArenaEntitySlot& slot,std::uint32_t frame){
 return slot[15]==63 && word(slot,0x80)==frame; // E24/E28 exact equality
}
ArenaEntityFinalization finalize_arena_entity_record(ArenaEntitySlot& slot,
 std::span<std::uint8_t,128> statistics,const ArenaEntityFinalizationContext& c,
 std::uint32_t frame){
 auto next=slot;std::array<std::uint8_t,128> stats;
 std::copy(statistics.begin(),statistics.end(),stats.begin());
 put(next,0x60,word(next,0x60)&~2u); // 65C4/65EC, before either source branch
 if(c.removal_590){
  if(c.removal_handled_593){slot=next;return ArenaEntityFinalization::AlreadyHandled;}
  // 6704 ->48C8: caller chooses the correct statistics block from +542.
  // The original ignores its return, including UnsupportedEntity for type1.
  try{register_arena_entity_statistics(stats,next,false,true,c.auxiliary_596);}
  catch(const std::invalid_argument&){return ArenaEntityFinalization::Invalid;}
  initialize_arena_entity_tombstone(next,frame); // 671C ->3400
  slot=next;std::copy(stats.begin(),stats.end(),statistics.begin());
  return ArenaEntityFinalization::Tombstone;
 }
 if(word(next,0x60)&4u){
  const auto health=std::bit_cast<float>(word(next,0x38));
  if(!normal(health) || !normal(c.previous_scalar_38_4e8))return ArenaEntityFinalization::Invalid;
  Rounding rounding;volatile float difference=c.previous_scalar_38_4e8-health;
  if(!normal(difference))return ArenaEntityFinalization::Invalid;
  // 6634..663C masks the low bit of changed_59A, not any nonzero byte.
  if((std::abs(float(difference))>std::bit_cast<float>(0x38d1b717u) || (c.changed_59a&1u))&&next[0x7f]==0)
   return ArenaEntityFinalization::NeedsChangeEffects; // 3940 first-display allocation unresolved
  // 3940/3994..39EC returns immediately when body7F is already nonzero.
  // Existing health-display ownership therefore permits later health commits.
 }
 if(!c.skip_spatial_6260)return ArenaEntityFinalization::NeedsSpatialRecovery;
 // 6674/6678 replace XYZ only. The fourth lanes hold identity/type metadata
 // and caller state, not homogeneous position/velocity coordinates.
 for(unsigned i=0;i<3;++i){
  if(!normal(c.position_480[i]) || !normal(c.velocity_4a0[i]))return ArenaEntityFinalization::Invalid;
  put(next,4*i,std::bit_cast<std::uint32_t>(c.position_480[i]));
  put(next,0x40+4*i,std::bit_cast<std::uint32_t>(c.velocity_4a0[i]));
 }
 slot=next;return ArenaEntityFinalization::Committed;
}
ArenaEntityFinalization finalize_arena_entity_update(ArenaEntityStorage& storage,
 std::uint16_t index,const ArenaEntitySlot& updated,std::span<std::uint8_t,128> statistics,
 const ArenaEntityFinalizationContext& c,std::uint32_t frame){
 const auto member=membership(storage,index);
 if(!member || (word(updated,12)>>16)!=index)return ArenaEntityFinalization::Invalid;
 auto next=updated;std::array<std::uint8_t,128> stats;
 std::copy(statistics.begin(),statistics.end(),stats.begin());
 const auto result=finalize_arena_entity_record(next,stats,c,frame);
 if(result!=ArenaEntityFinalization::Tombstone && result!=ArenaEntityFinalization::Committed &&
    result!=ArenaEntityFinalization::AlreadyHandled)return result;
 if(next[15]==63){
  // 790 wrapper updates only the marker at874 and skips1F8B0 for kind63 at894.
  // Keep prior cell membership until the explicit completed-job release phase.
  for(unsigned b=0;b<2;++b){
   storage.banks[b][index]=next;
   storage.cells[b][member->flat][member->offset[b]][3]=next[0x6f];
  }
 }else if(result==ArenaEntityFinalization::Committed || result==ArenaEntityFinalization::AlreadyHandled){
  const auto published=update_arena_entity(storage,index,next);
  if(published==ArenaEntityUpdate::CellFull)return ArenaEntityFinalization::CellFull;
  if(published!=ArenaEntityUpdate::Updated)return ArenaEntityFinalization::Invalid;
 }
 std::copy(stats.begin(),stats.end(),statistics.begin());return result;
}
std::vector<std::uint16_t> collect_arena_entity_tombstones(const ArenaEntityStorage& storage,std::uint32_t frame){
 if(storage.read_bank>1 || !storage.width || !storage.height || storage.width>24 || storage.height>24)
  throw std::invalid_argument("invalid tombstone collection state");
 std::vector<std::uint16_t> result;
 // This owned adapter's enumeration is deterministic cell/list order. Original
 // cross-job scheduler order is supplied by completed batches, not inferred.
 for(const auto& cell:storage.cells[storage.read_bank])for(const auto& entry:cell){
  const auto index=std::uint16_t(unsigned(entry[0])<<8|entry[1]);
  if(index>=storage.banks[storage.read_bank].size())throw std::invalid_argument("invalid tombstone member");
  if(arena_entity_tombstone_due(storage.banks[storage.read_bank][index],frame)){
   if(!membership(storage,index))throw std::invalid_argument("inconsistent tombstone membership");
   result.push_back(index);
  }
 }
 return result;
}
bool release_completed_arena_tombstones(ArenaEntityStorage& storage,
 std::span<const std::uint16_t> indices,std::uint32_t frame){
 // Stage the whole batch: no partial identity return if a later entry is stale
 // or duplicated. Original serial/order is explicit, not a guessed frame rate.
 auto next=storage;
 for(auto index:indices){
  if(index>=next.banks[0].size() || index>=next.banks[1].size() ||
     !arena_entity_tombstone_due(next.banks[0][index],frame))return false;
  const auto first=next.banks[0][index],second=next.banks[1][index];
  if(!retire_arena_entity(next,index))return false;
  // 29F8 returns identities to the pool; it does not zero entity memory.
  // Preserve retired bytes for the next original reused-slot constructor.
  next.banks[0][index]=first;next.banks[1][index]=second;
 }
 storage=std::move(next);return true;
}
}
