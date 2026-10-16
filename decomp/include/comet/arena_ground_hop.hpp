#pragma once
#include "comet/arena_state.hpp"
#include <array>
#include <optional>
namespace comet::decomp {
struct ArenaGroundHopQuery {
 std::array<float,4> velocity{},steering{},lookahead_steering{},external_acceleration{0,-1.5f,0,0};
 float timer=0,elapsed=0,speed_limit=1;
 std::uint32_t flags=0;
 bool grounded=false;
};
struct ArenaGroundHopResult {
 std::array<float,4> acceleration{},velocity{};
 float timer=0,horizontal_speed_squared_before_limit=0;
 std::optional<float> position_y;
 bool hopped=false,speed_limited=false;
};
// Kind-44 17260..175AC/17980..17A70, then velocity integration/limiting.
// On a hop, sample lookahead steering at (position with Y=-.1 when flags&2)
// plus current steering. The random argument is consumed once only on a hop.
std::optional<ArenaGroundHopResult> advance_arena_ground_hop(const ArenaGroundHopQuery &,ArenaRandom &);
}
