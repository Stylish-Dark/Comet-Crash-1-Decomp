#pragma once
#include "comet/arena_state.hpp"
#include <array>
#include <optional>
namespace comet::decomp {
// Explicit native inputs for the selected-kind/held-input portion of C730.
struct ArenaGroundDeploymentControl {
  std::uint8_t kind=40;
  float strength=0;
};
enum class ArenaGroundDeploymentGate { Inactive, Waiting, Ready, Invalid };
// CB04..CB10 and DA48..DA8C: increment timer, then compare strictly against
// fma((original per-kind interval - 2), strength, 2). No success reset here;
// DBE8 resets the timer only after a successful spawn. Invalid input is atomic.
ArenaGroundDeploymentGate advance_arena_ground_deployment(
    float &timer,ArenaGroundDeploymentControl,float elapsed);
// 1250..1260: job-cell center used as the original C380 spawn position.
std::optional<std::array<float,4>> arena_ground_deployment_position(ArenaGridPosition);
}
