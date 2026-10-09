#include "comet/arena_entity_storage.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
float read_float(std::span<const std::uint8_t, 256> bytes, unsigned off) {
  return std::bit_cast<float>((std::uint32_t(bytes[off]) << 24) |
                              (std::uint32_t(bytes[off + 1]) << 16) |
                              (std::uint32_t(bytes[off + 2]) << 8) |
                              bytes[off + 3]);
}
} // namespace
bool pack_arena_cell_entry(ArenaPackedCellEntry &entry,
                           std::span<const std::uint8_t, 256> entity,
                           std::uint8_t flags, std::uint8_t marker,
                           ArenaGridPosition &cell, std::uint32_t width,
                           std::uint32_t height) {
  if (width == 0 || height == 0 || width > 24 || height > 24)
    throw std::invalid_argument("invalid native cell extent");
  const std::array<float, 3> position{
      read_float(entity, 0), read_float(entity, 4), read_float(entity, 8)};
  for (const auto value : position)
    if (!std::isfinite(value) || static_cast<double>(value) < -2147483648.0 ||
        static_cast<double>(value) > 2147483647.0)
      throw std::invalid_argument("invalid native cell position");
  const ArenaGridPosition actual{
      static_cast<std::uint8_t>(std::clamp(std::trunc(position[0]), 0.0f,
                                           static_cast<float>(width - 1))),
      static_cast<std::uint8_t>(std::clamp(std::trunc(position[2]), 0.0f,
                                           static_cast<float>(height - 1)))};
  const std::array<float, 3> origin{static_cast<float>(cell.x) - 0.5f, -0.5f,
                                    static_cast<float>(cell.z) - 0.5f};
  ArenaPackedCellEntry result{entity[0xc], entity[0xd], entity[0xe], marker,
                              0,           0,           0,           flags};
  for (unsigned i = 0; i < 3; ++i) {
    const float local = position[i] - origin[i];
    result[4 + i] =
        static_cast<std::uint8_t>(std::clamp(local, 0.0f, 2.0f) * 100.0f);
  }
  const bool moved = actual != cell;
  entry = result;
  cell = actual;
  return moved;
}
std::optional<std::uint16_t>
insert_arena_entity(ArenaEntityStorage &storage, ArenaGridPosition cell,
                    std::span<const std::uint8_t, 256> entity_template) {
  if (storage.width == 0 || storage.height == 0 || storage.width > 24 ||
      storage.height > 24 || storage.read_bank > 1 || cell.x >= storage.width ||
      cell.z >= storage.height)
    throw std::invalid_argument("invalid native entity storage state");
  const auto flat = cell.z * 24 + cell.x;
  if (storage.cells[0][flat].size() >= 126 ||
      storage.cells[1][flat].size() >= 126 || storage.free_indices.empty())
    return {};
  const auto index = storage.free_indices.front();
  if (index > 65535 || index >= storage.banks[0].size() ||
      index >= storage.banks[1].size())
    throw std::invalid_argument("invalid native entity pool index");
  ArenaEntitySlot slot;
  std::copy(entity_template.begin(), entity_template.end(), slot.begin());
  slot[0xc] = static_cast<std::uint8_t>(index >> 8);
  slot[0xd] = static_cast<std::uint8_t>(index);
  std::array<ArenaPackedCellEntry, 2> entries;
  // Both list pointers are chosen before 1117F0 updates x/z. The second
  // packing call uses those updated coordinates, matching CA6F0/CA72C.
  pack_arena_cell_entry(entries[storage.read_bank], slot, slot[0x33], 40, cell,
                        storage.width, storage.height);
  pack_arena_cell_entry(entries[1 - storage.read_bank], slot, slot[0x33], 40,
                        cell, storage.width, storage.height);
  for (auto &bank : storage.cells)
    bank[flat].reserve(bank[flat].size() + 1);
  storage.free_indices.pop_front();
  for (unsigned i = 0; i < 2; ++i) {
    storage.banks[i][index] = slot;
    storage.cells[i][flat].push_back(entries[i]);
  }
  ++storage.summary_counts[cell.z * 24 + cell.x];
  return static_cast<std::uint16_t>(index);
}
ArenaEventTarget resolve_arena_storage_target(const ArenaEntityStorage &storage,
                                              std::uint8_t visibility,
                                              std::uint32_t mask,
                                              ArenaGridPosition cell) {
  if (storage.read_bank > 1 || cell.x >= storage.width ||
      cell.z >= storage.height || storage.width > 24 || storage.height > 24 ||
      cell.x >= 24 || cell.z >= 24)
    throw std::invalid_argument("invalid native cell lookup");
  const auto owner = static_cast<std::uint32_t>(visibility >> 4) - 1u;
  if (owner > 3 || ((mask >> owner) & 1u))
    return {};
  for (const auto &entry :
       storage.cells[storage.read_bank][cell.z * 24 + cell.x]) {
    const auto kind = entry[7] & 0x1c;
    if (kind == 0x0c || kind == 0x10 || ((mask >> (entry[7] >> 5)) & 1u))
      continue;
    const auto index = (std::uint16_t(entry[0]) << 8) | entry[1];
    if (static_cast<std::size_t>(index) >=
        storage.banks[storage.read_bank].size())
      throw std::invalid_argument("invalid native cell entity index");
    return {index, storage.banks[storage.read_bank][index][0xf]};
  }
  return {};
}
} // namespace comet::decomp
