#pragma once
#include "comet/arena_construction_progress.hpp"
#include "comet/arena_construction_runtime.hpp"
#include "comet/arena_entity_storage.hpp"
#include "comet/arena_factory_control.hpp"
#include "comet/arena_factory_production.hpp"
#include "comet/arena_structure_entity.hpp"
#include "comet/arena_unit_binding.hpp"
#include "comet/arena_unit_entity.hpp"
namespace comet::decomp {
struct ArenaConstructionAccount {
  ArenaRoutePlayer route;
  float resource = 0, resource_max = 0;
};
struct ArenaConstructionSimulationConfig {
  std::vector<ArenaConstructionAccount> players;
  std::uint32_t capacity = 0, seed = 1;
  std::array<std::optional<ArenaStructureTableValues>, 32> tuning{};
  std::array<std::uint8_t, 576> initial_visibility{};
  std::vector<ArenaEntity> initial_structures;
  std::vector<ArenaPlayer> initial_players;
  std::array<std::uint32_t, 4> event_exclusion_masks{};
  std::array<bool, 4> unit_base_available{};
  std::array<std::uint8_t, 4> unit_target_overrides{255, 255, 255, 255};
  bool production_enabled = false;
  std::array<ArenaUnitProductionQuota, 4> unit_quotas{};
};
bool native_map_structure_importable(const ArenaEntity &);
enum class ArenaConstructionSubmit {
  Queued,
  InvalidRequest,
  Occupied,
  CellFull,
  NoEntityCapacity,
  Duplicate,
  InsufficientResources,
  QueueFull
};
struct ArenaConstructionSimulationStep {
  ArenaConstructionStep status = ArenaConstructionStep::Idle;
  ArenaConstructionRequest request{};
  std::optional<std::uint16_t> entity_index;
};
enum class ArenaTimedSubmit { Scheduled, Invalid, Full };
enum class ArenaTimedStatus {
  Idle,
  Submitted,
  Retried,
  Unsupported,
  Backpressure
};
struct ArenaConstructionTick {
  std::array<ArenaTimedStatus, 4> players{};
  ArenaConstructionSimulationStep construction;
  std::vector<std::uint16_t> ready_entities;
  std::vector<std::uint16_t> factory_selections;
  std::vector<std::uint16_t> production_reservations;
};
class ArenaConstructionSimulation {
  ArenaConstructionSimulationConfig config_;
  ArenaConstructionQueue queue_;
  ArenaRoutingGrid grid_;
  ArenaEntityStorage storage_;
  std::vector<ArenaRoutePlayer> routes_;
  std::vector<ArenaPlayer> players_;
  ArenaRandom random_;
  float clock_ = 0;
  ArenaFactorySelection select_factory_index(std::int32_t, std::uint8_t);
  ArenaConstructionContext context_for(ArenaConstructionRequest) const;
  std::optional<ArenaEntitySlot>
  initialize_factory_unit_template(std::uint16_t, ArenaRandom &) const;

public:
  explicit ArenaConstructionSimulation(
      const ArenaConstructionSimulationConfig &);
  ArenaConstructionSubmit enqueue(ArenaConstructionRequest);
  ArenaConstructionSimulationStep step();
  ArenaFactorySelection select_factory(std::uint16_t entity_index,
                                       std::uint8_t selector);
  ArenaTimedSubmit enqueue_event(std::size_t, ArenaEvent);
  ArenaConstructionTick advance(float);
  // Inspection uses a copy of RNG; no quota claim, allocation or world
  // mutation.
  std::optional<ArenaEntitySlot>
  factory_unit_template(std::uint16_t index) const;
  std::optional<ArenaGroundUnitTemplate>
  factory_ground_unit_template(std::uint16_t index) const;
  const auto &unit_quotas() const { return config_.unit_quotas; }
  const auto &accounts() const { return config_.players; }
  const auto &players() const { return players_; }
  const auto &grid() const { return grid_; }
  const auto &storage() const { return storage_; }
  std::size_t pending() const { return queue_.size(); }
  float clock() const { return clock_; }
};
} // namespace comet::decomp
