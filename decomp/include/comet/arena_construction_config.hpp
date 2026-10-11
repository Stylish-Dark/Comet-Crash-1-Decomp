#pragma once
#include "comet/arena_construction_simulation.hpp"
#include <istream>
namespace comet::decomp {
// Strict, explicit tuning input; no unrecovered title tables are invented.
ArenaConstructionSimulationConfig
read_arena_construction_config(std::istream &, std::size_t &line);
} // namespace comet::decomp
