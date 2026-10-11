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

public:
  NativeConstructionScene(const ArenaState &,
                          const ArenaConstructionSimulationConfig &);
  ArenaConstructionSubmit enqueue(ArenaConstructionRequest);
  ArenaConstructionSimulationStep step();
  ArenaConstructionTick advance(float);
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
