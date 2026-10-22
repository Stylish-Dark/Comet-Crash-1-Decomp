#pragma once
#include "comet/arena_construction_simulation.hpp"
namespace comet::decomp {
struct ArenaMapImportSummary {
  std::size_t live = 0, display_only = 0;
};
class NativeConstructionScene {
  ArenaState arena_;
  ArenaConstructionSimulation simulation_;
  ArenaMapImportSummary imports_;
  static ArenaConstructionSimulationConfig
  bootstrap(const ArenaState &, ArenaConstructionSimulationConfig);
  void refresh_construction();
  void project(const ArenaConstructionSimulationStep &);
  void project_dynamic_entity(std::uint16_t index);

public:
  NativeConstructionScene(const ArenaState &,
                          const ArenaConstructionSimulationConfig &);
  ArenaConstructionSubmit enqueue(ArenaConstructionRequest);
  ArenaConstructionSimulationStep step();
  ArenaConstructionTick advance(float);
  ArenaCraftRegistration create_player_craft(std::uint8_t owner,
      const std::array<float,4>& spawn,const ArenaEntitySlot& reused,
      const ArenaCraftAuxiliaryStart&);
  bool stage_player_craft_preparation(std::uint8_t owner,
      std::span<const std::uint8_t,0x2900> snapshot,ArenaEntityWorkContext& work) const {
    return simulation_.stage_player_craft_preparation(owner,snapshot,work);
  }
  ArenaPreparationCompletion complete_player_craft_preparation(std::uint8_t owner,
      std::span<const std::uint8_t,0x2900> snapshot,const ArenaAccumulatorConsumption& effects,
      ArenaEntityWorkContext& work,ArenaEmissionDescriptor* scratch=nullptr,
      const ArenaCounterEmissionSink& emit={}) {
    return simulation_.complete_player_craft_preparation(owner,snapshot,effects,work,scratch,emit);
  }
  ArenaPlayerCraftControlStep advance_prepared_player_craft(std::uint8_t owner,
      std::span<const std::uint8_t,0x2900> snapshot,float elapsed,ArenaEntityWorkContext&,
      std::uint32_t original_frame,ArenaPlayerControlScratch* scratch=nullptr,
      const ArenaPlayerPendingTransport& pending={},const ArenaCounterEmissionSink& emitter={});
  // Resume retained controller output without replaying request callbacks.
  ArenaEntityFinalization finalize_player_craft(std::uint8_t owner,
      const ArenaEntitySlot& updated,const ArenaEntityFinalizationContext&,
      std::uint32_t original_frame);
  bool set_ground_deployment(std::size_t owner,ArenaGroundDeploymentControl control) { return simulation_.set_ground_deployment(owner,control); }
  void set_ground_contacts(bool enabled) { simulation_.set_ground_contacts(enabled); }
  void set_ground_dynamics(bool enabled) { simulation_.set_ground_dynamics(enabled); }
  void set_ground_transport(bool enabled) { simulation_.set_ground_transport(enabled); }
  bool set_ground_capture_ready(std::uint16_t index,bool ready) { return simulation_.set_ground_capture_ready(index,ready); }
  bool queue_ground_health_delta(std::uint16_t index,float delta) { return simulation_.queue_ground_health_delta(index,delta); }
  ArenaUnitSpawnResult spawn_reserved_ground_unit(std::uint8_t owner,
                                                  std::uint8_t kind,
                                                  ArenaGridPosition);
  ArenaFactorySelection select_factory(std::uint16_t entity_index,
                                       std::uint8_t selector);
  ArenaFactorySelection select_factory(std::size_t player, ArenaGridPosition,
                                       std::uint8_t selector);
  ArenaTimedSubmit enqueue_event(std::size_t owner, ArenaEvent e) {
    if (e.selector < 9 &&
        (e.grid_x >= arena_.extent || e.grid_z >= arena_.extent))
      return ArenaTimedSubmit::Invalid;
    return simulation_.enqueue_event(owner, e);
  }
  const ArenaState &arena() const { return arena_; }
  const ArenaConstructionSimulation &simulation() const { return simulation_; }
  ArenaMapImportSummary imports() const { return imports_; }
};
} // namespace comet::decomp
