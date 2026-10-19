#pragma once
#include "comet/arena_craft_flight.hpp"
#include <cstdint>
#include <span>
namespace comet::decomp {
// PPU CFAB8..CFB88 slot writes, including FECFC. Allocation/registration and
// player links are excluded. Spawn Y is zero; staged W copies only to +EC.
// Reused-slot bytes not written by the original remain intact.
void initialize_arena_player_craft_entity(std::span<std::uint8_t,256>,
 std::uint8_t owner,float x,float z,float staged_w=0);
// Original type-1 constructor fields staged by SPU 9A70 for 7DD0 containment.
// Extents 1..24 are the native supported domain. Restores caller rounding.
ArenaCraftBounds arena_player_craft_bounds(unsigned width,unsigned depth);
}
