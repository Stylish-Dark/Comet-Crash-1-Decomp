#include "comet/arena_entity_storage.hpp"
#include "comet/arena_structure_entity.hpp"
#include <bit>
#include <stdexcept>
using namespace comet::decomp;
static void check(bool v) {
  if (!v)
    throw std::runtime_error("entity storage check failed");
}
static void put_float(ArenaEntitySlot &b, unsigned o, float f) {
  auto n = std::bit_cast<std::uint32_t>(f);
  for (unsigned i = 0; i < 4; ++i)
    b[o + i] = std::uint8_t(n >> (24 - 8 * i));
}
int main() {
  ArenaEntitySlot slot;
  slot.fill(0xa5);
  initialize_arena_type21_entity(slot, 2, {8, 12}, {50, 10, 25, 10});
  slot[0xc] = 0x12;
  slot[0xd] = 0x34;
  ArenaGridPosition cell{8, 12};
  ArenaPackedCellEntry entry;
  check(!pack_arena_cell_entry(entry, slot, 0x50, 40, cell, 24, 24));
  check(entry == ArenaPackedCellEntry{0x12, 0x34, 0, 40, 100, 50, 100, 0x50});
  put_float(slot, 0, -3.5f);
  put_float(slot, 4, 5);
  put_float(slot, 8, 99.75f);
  cell = {8, 12};
  check(pack_arena_cell_entry(entry, slot, 0xff, 7, cell, 10, 20));
  check(cell == ArenaGridPosition{0, 19});
  check(entry == ArenaPackedCellEntry{0x12, 0x34, 0, 7, 0, 200, 200, 0xff});
  ArenaEntityStorage storage;
  storage.banks[0].resize(5);
  storage.banks[1].resize(5);
  storage.free_indices = {3, 1};
  storage.summary_counts[12 * 24 + 8] = 65535;
  initialize_arena_type21_entity(slot, 0, {8, 12}, {50, 10, 25, 10});
  check(insert_arena_entity(storage, {8, 12}, slot) == 3);
  check(storage.free_indices.size() == 1 && storage.free_indices.front() == 1);
  check(storage.banks[0][3] == storage.banks[1][3]);
  check(storage.banks[0][3][0xc] == 0 && storage.banks[0][3][0xd] == 3);
  check(storage.banks[0][3][0xe] == 0 && storage.banks[0][3][0xf] == 21);
  check(storage.cells[0][12 * 24 + 8][0] ==
        ArenaPackedCellEntry{0, 3, 0, 40, 100, 50, 100, 0x10});
  check(storage.cells[0][12 * 24 + 8] == storage.cells[1][12 * 24 + 8]);
  check(storage.summary_counts[12 * 24 + 8] == 0);
  check(insert_arena_entity(storage, {8, 12}, slot) == 1);
  check(storage.cells[0][12 * 24 + 8][0][1] == 3 &&
        storage.cells[0][12 * 24 + 8][1][1] == 1);
  check(!insert_arena_entity(storage, {8, 12}, slot));
  storage.free_indices = {4};
  storage.cells[1][12 * 24 + 8].resize(126);
  check(!insert_arena_entity(storage, {8, 12}, slot));
  check(storage.free_indices.front() == 4 &&
        storage.cells[0][12 * 24 + 8].size() == 2);
  check(storage.summary_counts[12 * 24 + 8] == 1);
  ArenaEntityStorage moved;
  moved.read_bank = 1;
  moved.banks[0].resize(1);
  moved.banks[1].resize(1);
  moved.free_indices = {0};
  check(insert_arena_entity(moved, {4, 4}, slot) == 0);
  // CA514 selects both list pointers before the first helper adjusts x/z.
  check(moved.cells[1][4 * 24 + 4][0][4] == 200 &&
        moved.cells[1][4 * 24 + 4][0][6] == 200);
  check(moved.cells[0][4 * 24 + 4][0][4] == 100 &&
        moved.cells[0][4 * 24 + 4][0][6] == 100);
  check(moved.summary_counts[12 * 24 + 8] == 1 &&
        moved.summary_counts[4 * 24 + 4] == 0);
  check(resolve_arena_storage_target(moved, 0x1f, 0, {4, 4}).entity_index ==
        -1);
  moved.cells[1][4 * 24 + 4][0][7] = 0;
  auto target = resolve_arena_storage_target(moved, 0x1f, 0, {4, 4});
  check(target.entity_index == 0 && target.type == 21);
  check(resolve_arena_storage_target(moved, 0x1f, 1, {4, 4}).entity_index ==
        -1);
  moved.read_bank = 0;
  check(resolve_arena_storage_target(moved, 0x1f, 0, {4, 4}).entity_index ==
        -1);
}
