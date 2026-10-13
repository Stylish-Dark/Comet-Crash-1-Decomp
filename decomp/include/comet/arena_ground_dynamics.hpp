#pragma once
#include <array>
#include <cstdint>
#include <optional>
namespace comet::decomp {
// Recovered FE5D8 windows 15998..159DC and 15C38..15C80/162B0..162FC.
// Proven for finite normal arithmetic (and zero lanes), not SPU denormal or
// extended-exponent behavior. Caller preparation, collisions and special-kind
// branches are separate.
struct ArenaGroundDynamicsQuery {
  std::array<float, 4> velocity{}, steering{}, external_acceleration{};
  float steering_acceleration = 0, speed_limit = 0, elapsed = 0;
  std::uint32_t context_flags = 0;
  std::uint8_t kind = 40;
};
struct ArenaGroundDynamicsResult {
  std::array<float, 4> acceleration{}, velocity{};
  float horizontal_speed_squared_before_limit = 0;
  bool speed_limited = false;
};
std::optional<ArenaGroundDynamicsResult>
advance_arena_ground_velocity(const ArenaGroundDynamicsQuery &);
} // namespace comet::decomp
