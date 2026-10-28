#pragma once
#include "comet/arena_entity_storage.hpp"
namespace comet::decomp {
struct ArenaSpatialNeighborhood {
 int cell_x=0,cell_z=0; // original20DC0/20DD0 job cell, before entity migration
 // Original20D80 blocks: x offset +3*z offset, ordered packed records.
 std::array<std::span<const ArenaPackedCellEntry>,9> cells;
 // For odd counts, eight reused bytes after the last record affect scratch
 // lanes. Odd-index records rotate within their aligned16-byte pair.
 std::array<ArenaPackedCellEntry,9> tail{};
};
// Original6568 ->6260 nearest eligible peer, body1C +work5B0. Source caches
// body10 (category mask),4E0 (own radius),4F0 (own identity) are explicit.
// Reject unsupported numeric inputs transactionally. No scheduler is chosen.
bool select_arena_entity_peer(std::span<std::uint8_t> work,const ArenaSpatialNeighborhood&);
// Serialized native bank view for an explicitly supplied original job cell.
ArenaSpatialNeighborhood arena_spatial_neighborhood(const ArenaEntityStorage&,ArenaGridPosition);
}
