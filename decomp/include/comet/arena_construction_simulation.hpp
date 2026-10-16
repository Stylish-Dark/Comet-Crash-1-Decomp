#pragma once
#include "comet/arena_construction_progress.hpp"
#include "comet/arena_construction_runtime.hpp"
#include "comet/arena_entity_storage.hpp"
#include "comet/arena_factory_control.hpp"
#include "comet/arena_factory_production.hpp"
#include "comet/arena_structure_entity.hpp"
#include "comet/arena_unit_binding.hpp"
#include "comet/arena_unit_entity.hpp"
#include "comet/arena_unit_spawn.hpp"
#include "comet/arena_ground_transport.hpp"
#include "comet/arena_ground_deployment.hpp"
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
  // Explicit native bootstrap; absent means base impacts are disabled.
  std::array<std::optional<std::int16_t>,4> base_counters{};
  std::array<std::optional<ArenaGroundDeploymentControl>,4> ground_deployment{};
  bool production_enabled = false;
  bool tesla_enabled = false;
  bool ground_transport_enabled = false;
  bool ground_dynamics_enabled = false;
  bool ground_contacts_enabled = false;
  std::uint8_t native_extent = 24;
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
struct ArenaGroundBaseImpact {
  std::uint16_t entity_index=0;
  std::uint8_t base=0,owner=0;
};
struct ArenaGroundDeploymentAttempt {
  std::uint8_t owner=0,kind=40;
  ArenaUnitSpawnResult result{};
};
struct ArenaConstructionTick {
  std::array<ArenaTimedStatus, 4> players{};
  ArenaConstructionSimulationStep construction;
  std::vector<std::uint16_t> ready_entities;
  std::vector<std::uint16_t> factory_selections;
  std::vector<std::uint16_t> production_reservations;
  std::vector<std::uint16_t> transported_units;
  std::vector<ArenaGroundBaseImpact> base_impacts;
  std::vector<std::uint16_t> depleted_units, retired_entities;
  std::vector<std::uint16_t> ground_slams;
  std::vector<std::uint16_t> spawned_beams;
  std::vector<ArenaGroundDeploymentAttempt> ground_deployments;
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
  std::array<float,4> deployment_timers_{};
  // Native-owned replacement for the health lane of each original accumulator.
  // Consumed once when the active ground simulation next advances.
  std::vector<float> health_deltas_;
  std::vector<std::uint32_t> tesla_commands_;
  std::array<std::array<std::uint8_t, 128>, 4> statistics_{};
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
  ArenaUnitSpawnResult
  spawn_reserved_ground_unit(std::uint8_t owner, std::uint8_t kind,
                             const std::array<float, 4> &position);
  // Explicit native preview control for the recovered capture-readiness byte.
  bool set_ground_capture_ready(std::uint16_t index,bool ready);
  bool queue_ground_health_delta(std::uint16_t index,float delta);
  std::optional<std::int16_t> base_counter(std::size_t owner) const;
  bool set_ground_deployment(std::size_t owner,ArenaGroundDeploymentControl);
  const auto &ground_deployment() const { return config_.ground_deployment; }
  const auto &deployment_timers() const { return deployment_timers_; }
  const auto &player_statistics() const { return statistics_; }
  const auto &unit_quotas() const { return config_.unit_quotas; }
  const auto &accounts() const { return config_.players; }
  const auto &players() const { return players_; }
  const auto &grid() const { return grid_; }
  const auto &storage() const { return storage_; }
  std::size_t pending() const { return queue_.size(); }
  float clock() const { return clock_; }
  void set_ground_transport(bool enabled) { config_.ground_transport_enabled = enabled; }
  bool ground_transport_enabled() const { return config_.ground_transport_enabled; }
  void set_ground_dynamics(bool enabled) { config_.ground_dynamics_enabled = enabled; if(!enabled)config_.ground_contacts_enabled=false; }
  bool ground_dynamics_enabled() const { return config_.ground_dynamics_enabled; }
  void set_ground_contacts(bool enabled) { config_.ground_contacts_enabled=enabled;if(enabled)config_.ground_dynamics_enabled=true; }
  bool ground_contacts_enabled() const { return config_.ground_contacts_enabled; }
};
} // namespace comet::decomp
