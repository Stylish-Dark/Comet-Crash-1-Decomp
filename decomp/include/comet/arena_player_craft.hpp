#pragma once
#include "comet/arena_craft_flight.hpp"
#include "comet/arena_entity_storage.hpp"
#include <functional>
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
// PPU CF2A0..CF2BC/CFFC8..D0020: use the selected configured X/Z if
// X>=0; otherwise use the original four-player center arrangement. The root
// mode's selection of which configured pair to supply remains a caller input.
std::optional<std::array<float,4>> resolve_arena_player_craft_spawn(
 std::uint8_t owner,unsigned width,unsigned depth,float configured_x,
 float configured_z,float staged_w=0);
// PPU CFA30..CFC0C: the caller has selected the original spawn X/Z.
// 1326C0 is an audio-system boundary (44,100 Hz voice setup downstream).
// Category meanings and scalar semantics are deliberately not guessed.
struct ArenaCraftAuxiliaryRequest {
 std::uint32_t category=0, x=0, z=0;
 std::uint8_t flag=1;
 float scalar=0;
 bool operator==(const ArenaCraftAuxiliaryRequest&) const = default;
};
using ArenaCraftAuxiliaryStart = std::function<std::int32_t(const ArenaCraftAuxiliaryRequest&)>;
struct ArenaPlayerCraftRegistry {
 // PPU root+3224+2*owner, mirrored as native optional indices.
 std::array<std::optional<std::uint16_t>,4> indices{};
};
enum class ArenaCraftRegistration { Registered, Invalid, AlreadyLinked, NoCapacity, MissingAuxiliary };
// Native admission checks reject before source side effects. Once admitted,
// auxiliary requests and player-link writes retain source order. The required
// callback must not throw, mutate storage, or re-enter registration. A returned
// -1 is retained exactly; it does not undo original entity registration.
ArenaCraftRegistration register_arena_player_craft(ArenaEntityStorage&, ArenaPlayer&,
 ArenaPlayerCraftRegistry&, std::uint8_t owner, const std::array<float,4>& spawn,
 const ArenaEntitySlot& reused_slot, const ArenaCraftAuxiliaryStart&);

}
