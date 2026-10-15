#pragma once
#include "comet/arena_construction_simulation.hpp"
#include <cstddef>
#include <cstdint>
namespace comet::decomp {
// Native mission-preview victory condition, not yet recovered title win/loss code.
enum class NativeMissionOutcome : std::uint8_t { Unconfigured, InProgress, Victory, Defeat, Draw };
NativeMissionOutcome evaluate_native_mission(const ArenaConstructionSimulation& simulation,
                                            std::size_t perspective_owner);
}
