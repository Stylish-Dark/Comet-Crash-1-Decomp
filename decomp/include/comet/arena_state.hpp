#pragma once
#include "comet/level_map.hpp"
#include "comet/model_load_parameters.hpp"
#include <array>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>
namespace comet::decomp {
inline constexpr std::size_t kArenaGridStride = 24;
struct ArenaGridPosition {
  std::uint8_t x = 0, z = 0;
  bool operator==(const ArenaGridPosition &) const = default;
};
// Runtime secondary records compress from 24 to 8 bytes at 0xD7BCC..0xD7C88.
struct ArenaEvent {
  float time = 0;
  std::uint8_t grid_x = 0, grid_z = 0, selector = 0, opcode = 0;
};
static_assert(sizeof(ArenaEvent) == 8);
class ArenaEventQueue {
  std::vector<ArenaEvent> heap_;

public:
  void push(ArenaEvent event);
  ArenaEvent pop(); // throws on empty
  const ArenaEvent &next() const;
  bool empty() const { return heap_.empty(); }
  std::size_t size() const { return heap_.size(); }
};
// 0x198760 shuffled LCG; state is per native simulation, rather than PPU TLS.
class ArenaRandom {
  std::uint32_t seed_ = 1, selector_ = 0;
  std::array<std::uint32_t, 32> shuffle_{};
  bool initialized_ = false;
  std::uint32_t step();

public:
  explicit ArenaRandom(std::uint32_t seed = 1) : seed_(seed) {}
  void seed(std::uint32_t value) {
    seed_ = value;
    initialized_ = false;
  }
  std::uint32_t next();
};
struct ArenaPendingCommand {
  std::int32_t entity_index = -1;
  std::uint8_t selector = 11, opcode = 0, grid_x = 0, grid_z = 0,
               source_flag = 0;
};
struct ArenaPlayer {
  std::optional<ArenaGridPosition> start_grid;
  ArenaEventQueue events;
  // Proven dispatch gates: player +0x23B and +0x4C respectively.
  bool dispatch_blocked = false;
  std::uint8_t command_mode = 11;
  ArenaPendingCommand pending;
  std::size_t event_capacity =
      4096; // native policy; caller may set recovered capacity
};
struct ArenaEntity {
  std::uint8_t type = 0, model_slot = 0, initial_upgrade = 0, variant = 0;
  std::uint32_t owner = 0;
  ArenaGridPosition grid;
  ModelPosition position;
  // Map +0x04..+0x10 -> entity +0x20..+0x2C, preserved in source order.
  std::array<float, 4> orientation{0, 0, 0, 1};
  std::size_t source_record = 0;
};
struct ArenaCell {
  std::optional<std::size_t> entity_index;
  std::uint8_t visibility = 0;
  // Gateway visibility depends on a player team field not recovered here.
  bool visibility_recovered = true;
};
struct ArenaState {
  std::uint32_t extent = 24;
  std::uint8_t theme = 0;
  std::vector<ArenaPlayer> players;
  std::vector<ArenaEntity> entities;
  std::array<ArenaCell, kArenaGridStride * kArenaGridStride> cells{};
  std::vector<std::size_t> unsupported_records;
  ModelPosition background_position{12, -0.2f, 12};
  std::array<float, 4> background_orientation{0, 0, 0, 1};
  const ArenaCell *cell(std::uint32_t x, std::uint32_t z) const;
};
struct ArenaLoadConfig {
  std::uint32_t player_count = 4, primary_player = 0;
  int secondary_player = 1;
  std::uint32_t level_id = 0;
};
struct ArenaStateResult {
  bool ok = true;
  std::string detail;
  explicit operator bool() const { return ok; }
};
// Native semantic replacement for the supported construction/routing boundaries
// in 0xD7A48. Economic, upgrade and constructor-specific state remains
// separate.
ArenaStateResult initialize_arena_state(const LevelMapData &,
                                        const ArenaLoadConfig &, ArenaState &);
// 0xF1030 consumes at most one event, and only when event.time < clock.
std::optional<ArenaEvent> pop_due_arena_event(ArenaPlayer &, float clock);
std::size_t schedule_arena_event(ArenaPlayer &, ArenaEvent, float clock,
                                 ArenaRandom &, std::int32_t extra_count = 0);
struct ArenaEventTarget {
  std::int32_t entity_index = -1;
  std::uint8_t type = 0;
};
struct ArenaCellEntry {
  std::uint16_t entity_index = 0;
  std::uint8_t flags = 0;
};
// 0xCA2E0 ordered lookup; caller selects the active bank and exclusion mask.
std::int32_t resolve_arena_cell_target(std::uint8_t visibility,
                                       std::uint32_t exclusion_mask,
                                       std::span<const ArenaCellEntry>);
using ArenaTargetResolver =
    std::function<ArenaEventTarget(std::uint8_t, std::uint8_t)>;
enum class ArenaDispatchResult { Idle, Pending, Retried };
// Resolver must implement 0xCA2E0 visibility/mask/list semantics; occupancy
// alone is insufficient. This boundary permits verified native state to supply
// it.
ArenaDispatchResult dispatch_arena_event(ArenaPlayer &, float, ArenaRandom &,
                                         const ArenaTargetResolver &);
std::optional<ArenaPendingCommand> take_arena_pending_command(ArenaPlayer &);
std::optional<std::uint8_t> initial_model_slot_for_record(std::uint8_t type);
} // namespace comet::decomp
