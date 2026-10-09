#include "comet/arena_construction.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
bool ArenaConstructionQueue::push(ArenaConstructionRequest entry) {
  // Bounded native policy: the original producer has no full-ring guard.
  if (count_ == entries_.size())
    return false;
  entries_[(head_ + count_) % entries_.size()] = entry;
  ++count_;
  return true;
}
ArenaConstructionRequest ArenaConstructionQueue::pop() {
  if (empty())
    throw std::out_of_range("empty construction queue");
  auto entry = entries_[head_];
  head_ = (head_ + 1) % entries_.size();
  --count_;
  return entry;
}
bool ArenaConstructionQueue::contains_grid(std::uint8_t x,
                                           std::uint8_t z) const {
  for (std::size_t i = 0; i < count_; ++i) {
    const auto &entry = entries_[(head_ + i) % entries_.size()];
    if (entry.grid_x == x && entry.grid_z == z)
      return true;
  }
  return false;
}
std::optional<std::uint16_t> arena_construction_cost(std::uint8_t opcode) {
  switch (opcode) {
  case 9:
    return 1;
  case 20:
    return 20;
  case 21:
    return 10;
  case 22:
    return 50;
  case 23:
    return 25;
  case 24:
    return 40;
  case 25:
    return 10;
  case 26:
    return 60;
  case 28:
    return 150;
  case 29:
    return 0;
  default:
    return {};
  }
}
ArenaConstructionError
validate_arena_construction(const ArenaConstructionRequest &request,
                            const ArenaConstructionContext &context,
                            const ArenaConstructionQueue &queue) {
  const auto cost = arena_construction_cost(request.opcode);
  if (!cost || request.player >= 4 || request.grid_x >= 24 ||
      request.grid_z >= 24 || !std::isfinite(context.resource) ||
      !std::isfinite(context.resource_max))
    return ArenaConstructionError::InvalidRequest;
  if (!context.override_occupancy && (context.visibility & 0xf0) != 0)
    return ArenaConstructionError::Occupied;
  if (context.cell_entry_count == 126)
    return ArenaConstructionError::CellFull;
  if (context.available_entities == 0)
    return ArenaConstructionError::NoEntityCapacity;
  if (queue.contains_grid(request.grid_x, request.grid_z))
    return ArenaConstructionError::Duplicate;
  if (context.resource <
      (context.cost_override ? 1.0f : static_cast<float>(*cost)))
    return ArenaConstructionError::InsufficientResources;
  return ArenaConstructionError::None;
}
bool commit_arena_construction(
    const ArenaConstructionRequest &request, ArenaConstructionContext &context,
    const ArenaConstructionQueue &queue,
    const std::function<bool(ArenaConstructionRequest)> &construct) {
  if (validate_arena_construction(request, context, queue) !=
      ArenaConstructionError::None)
    return false;
  const auto cost =
      context.cost_override
          ? 1.0f
          : static_cast<float>(*arena_construction_cost(request.opcode));
  if (!construct(request))
    return false;
  context.resource = std::min(context.resource - cost, context.resource_max);
  return true;
}
} // namespace comet::decomp
