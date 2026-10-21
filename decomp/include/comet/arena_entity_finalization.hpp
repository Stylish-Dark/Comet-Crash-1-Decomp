#pragma once
#include "comet/arena_entity_storage.hpp"
#include "comet/arena_player_statistics.hpp"
namespace comet::decomp {
// Named source context lanes consumed by FE5D8 6578. W lanes are not copied
// into packed body metadata. The original frame serial is supplied, not derived
// from render frames or elapsed seconds.
struct ArenaEntityFinalizationContext {
 std::array<float,4> position_480{}, velocity_4a0{};
 float previous_scalar_38_4e8=0;
 std::uint16_t statistics_owner_542=0;
 std::uint8_t removal_590=0, removal_handled_593=0, auxiliary_596=0, changed_59a=0;
 bool skip_spatial_6260=true; // nonzero r4; original type1 wrapper passes1
};
enum class ArenaEntityFinalization {
 Committed, Tombstone, AlreadyHandled, NeedsChangeEffects, NeedsSpatialRecovery,
 Invalid, CellFull
};
// Original 3400 ->2DF8 fixed constructor. Preserves unwritten reused bytes,
// including entity identity, and writes wrapping frame+3 to body+80.
void initialize_arena_entity_tombstone(ArenaEntitySlot&,std::uint32_t frame);
// Original E00..E84 admission: kind63 AND deadline == frame (not >=).
bool arena_entity_tombstone_due(const ArenaEntitySlot&,std::uint32_t frame);
// 6578 live/removal finalization. Returns a recovery boundary without writes
// if unrecovered3940/6260 is required. It does not pretend those effects ran.
ArenaEntityFinalization finalize_arena_entity_record(ArenaEntitySlot&,
 std::span<std::uint8_t,128> statistics,const ArenaEntityFinalizationContext&,
 std::uint32_t frame);
// Synchronous owned-bank publication; original DMA scheduling remains outside.
// Tombstones retain their original cell membership until job completion.
ArenaEntityFinalization finalize_arena_entity_update(ArenaEntityStorage&,
 std::uint16_t index,const ArenaEntitySlot& updated,
 std::span<std::uint8_t,128> statistics,const ArenaEntityFinalizationContext&,
 std::uint32_t frame);
// Collect in cell/entry order. Does not release pool identities. Caller must
// complete the original job phase before applying this result.
std::vector<std::uint16_t> collect_arena_entity_tombstones(
 const ArenaEntityStorage&,std::uint32_t frame);
// Explicit completed-job boundary, preserves supplied order in the FIFO pool.
// Rejects invalid/stale/duplicate batches atomically; no automatic wall clock.
// Preserves retired bank bytes (29F8 does not clear them).
bool release_completed_arena_tombstones(ArenaEntityStorage&,
 std::span<const std::uint16_t> indices,std::uint32_t frame);
}
