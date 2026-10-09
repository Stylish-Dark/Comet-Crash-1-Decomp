#pragma once
#include "comet/arena_state.hpp"
#include <deque>
namespace comet::decomp {
using ArenaEntitySlot = std::array<std::uint8_t, 256>;
using ArenaPackedCellEntry = std::array<std::uint8_t, 8>;
struct ArenaEntityStorage {
  // Bank selection is supplied by the recovered simulation controller.
  std::array<std::vector<ArenaEntitySlot>, 2> banks;
  std::array<std::array<std::vector<ArenaPackedCellEntry>, 576>, 2> cells;
  // Pool ordering comes from bootstrap; 130A1C consumes one index FIFO.
  std::deque<std::uint32_t> free_indices;
  std::array<std::uint16_t, 576> summary_counts{};
  std::uint32_t width = 24, height = 24;
  std::uint8_t read_bank = 0;
};
// CAF20 pool portion: ascending indices with head zero and all slots available.
// Capacity is root+2D44F0; bank allocation and other reset fields are separate.
void reset_arena_entity_index_pool(ArenaEntityStorage &,
                                   std::uint32_t capacity);
// 1117F0 writes all eight bytes and clamps the entity's truncated x/z to the
// configured extent. Returns whether the supplied cell coordinate changed.
bool pack_arena_cell_entry(ArenaPackedCellEntry &,
                           std::span<const std::uint8_t, 256>,
                           std::uint8_t flags, std::uint8_t marker,
                           ArenaGridPosition &, std::uint32_t width,
                           std::uint32_t height);
// CA514 copies a slot to both banks, stores its index at +0x0C, appends ordered
// cell records to both banks and increments the final cell's summary counter.
// Null means full cell or empty pool. Invalid native state throws before
// writes.
std::optional<std::uint16_t>
insert_arena_entity(ArenaEntityStorage &, ArenaGridPosition,
                    std::span<const std::uint8_t, 256> entity_template);
// CA2E0 lookup over the selected bank's actual ordered packed records.
ArenaEventTarget resolve_arena_storage_target(const ArenaEntityStorage &,
                                              std::uint8_t visibility,
                                              std::uint32_t exclusion_mask,
                                              ArenaGridPosition);
} // namespace comet::decomp
