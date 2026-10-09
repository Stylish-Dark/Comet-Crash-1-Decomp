#pragma once
#include "comet/arena_construction.hpp"
#include "comet/arena_routes.hpp"
namespace comet::decomp {
enum class ArenaConstructionStep {
  Idle,
  InvalidRequest,
  RouteRejected,
  Constructed,
  ReleaseQueued,
  ReleaseQueueFull,
  Released
};
// CAB90/FDE54 reservation followed by CCC70/CCD2C completion. The constructor
// callback must supply the recovered type-specific entity/allocation subsystem.
// Retry scheduling and the insufficient-funds notification path are separate.
ArenaConstructionStep process_next_arena_construction(
    ArenaConstructionQueue &, ArenaRoutingGrid &, std::span<ArenaRoutePlayer>,
    std::span<ArenaPlayer>, ArenaConstructionContext &,
    const std::function<bool(ArenaConstructionRequest)> &);
} // namespace comet::decomp
