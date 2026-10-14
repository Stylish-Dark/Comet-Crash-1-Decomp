#pragma once
#include "comet/arena_state.hpp"
#include <array>
#include <optional>
namespace comet::decomp {
// Original 1F030 random-direction arithmetic, consuming two shuffled-LCG draws.
std::array<float,4> arena_ground_random_direction(ArenaRandom &random);
struct ArenaGroundPairQuery {
 std::array<float,4> position{},peer_position{},jitter{};
 float radius=.1f,own_mass=1,peer_mass=1;
 std::uint8_t peer_radius=10;
 std::uint16_t index=0,peer_index=1;
};
struct ArenaGroundPairResult {
 std::array<float,4> position{},normal{};
 float gap=0,response_share=0;
 bool overlapping=false;
};
// Motion geometry from 7E40; caller supplies 1F030 jitter, owns neighbor policy.
std::optional<ArenaGroundPairResult> prepare_arena_ground_pair(const ArenaGroundPairQuery &query);
}
