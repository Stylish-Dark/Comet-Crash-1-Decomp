#pragma once
#include "comet/arena_entity_storage.hpp"
namespace comet::decomp {
enum class ArenaGroundDeath { Alive, Corpse, Retire, Unsupported, Invalid };
// Depleted-health ordinary ground portion of 6F40/4B68/6578. Unregisters
// exactly once; flag 0x08000000 requests direct retirement. Notifications/FX
// and the asynchronous kind-63 tombstone are outside this leaf.
ArenaGroundDeath begin_arena_ground_death(ArenaEntitySlot &,
                                          std::span<std::uint8_t,128> statistics);
enum class ArenaGroundCorpseStep { Active, Expired, Unsupported, Invalid };
// 12DC8..12EE4: original kind-62 drift, age, velocity decay and scalar fields.
// Expiry is strict new_age > duration. Caller owns removal, cells and pool.
// Invalid native numeric input leaves the slot unchanged.
ArenaGroundCorpseStep advance_arena_ground_corpse(ArenaEntitySlot &,float elapsed);
// Synchronous owned adapter: cell-full rejection suppresses positional drift
// while age/decay progress; expired corpses release their index FIFO.
ArenaGroundCorpseStep advance_arena_ground_corpse(ArenaEntityStorage &,
                                                 std::uint16_t index,float elapsed);
}
