#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
namespace comet::decomp {
// Original ring entry at root+0x2D2900: owner, constructor opcode, x, z.
struct ArenaConstructionRequest {
  std::uint8_t player = 0, opcode = 0, grid_x = 0, grid_z = 0;
};
static_assert(sizeof(ArenaConstructionRequest) == 4);
class ArenaConstructionQueue {
  std::array<ArenaConstructionRequest, 256> entries_{};
  std::size_t head_ = 0, count_ = 0;

public:
  bool push(ArenaConstructionRequest);
  ArenaConstructionRequest pop();
  bool contains_grid(std::uint8_t, std::uint8_t) const;
  bool empty() const { return count_ == 0; }
  std::size_t size() const { return count_; }
};
struct ArenaConstructionContext {
  std::uint8_t visibility = 0;
  std::uint32_t cell_entry_count = 0;
  std::uint32_t available_entities = 0; // original root+0xAB30
  float resource = 0, resource_max = 0;
  bool override_occupancy = false, cost_override = false;
};
enum class ArenaConstructionError {
  None,
  InvalidRequest,
  Occupied,
  CellFull,
  NoEntityCapacity,
  Duplicate,
  InsufficientResources
};
// 0xD6088 cost table at 0x1BEAA0. Unsupported constructors have no cost.
std::optional<std::uint16_t> arena_construction_cost(std::uint8_t opcode);
ArenaConstructionError
validate_arena_construction(const ArenaConstructionRequest &,
                            const ArenaConstructionContext &,
                            const ArenaConstructionQueue &);
// Allocation/constructor state is supplied by its recovered subsystem. Debit
// occurs only after a successful commit, through the 0x130DF0 resource
// boundary.
bool commit_arena_construction(
    const ArenaConstructionRequest &, ArenaConstructionContext &,
    const ArenaConstructionQueue &,
    const std::function<bool(ArenaConstructionRequest)> &);
} // namespace comet::decomp
