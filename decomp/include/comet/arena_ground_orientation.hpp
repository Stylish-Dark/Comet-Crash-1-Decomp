#pragma once
#include <array>
#include <cstdint>
#include <optional>
namespace comet::decomp {
struct ArenaGroundOrientationQuery {
 std::array<float,4> velocity{},steering_direction{},quaternion{0,0,0,1};
 float prelimit_speed_squared=0,elapsed=0,turn_rate=1,bank_scale=0;
 std::uint8_t kind=40;
};
// Ordinary FE5D8 orientation with finite XYZ/W=0 directions and a unit prior.
// At prelimit speed squared <= bits 0x3e4ccccc (~.2), steering_direction supplies facing.
// bank_scale is tuning +0x10; turn_rate is entity +0x88.
// Zero directions retain the prior; degenerate bank bases use zero bank.
// Kind 46 has a separate, unsupported policy.
std::optional<std::array<float,4>> prepare_arena_ground_orientation(const ArenaGroundOrientationQuery &query);
}

namespace comet::decomp {
//16B68..16DE4: parent2590 bank ->2250 roll, original shortest-arc blend.
// Shares the existing original trig/estimate arithmetic and its documented
// degenerate-bank/hardware exclusions. Inputs are source body20, parentA20,
// elapsed, bodyE8 (turn rate) and body90 (bank scale).
std::optional<std::array<float,4>> prepare_arena_ground_carried_orientation(
 const std::array<float,4>& prior,const std::array<float,4>& parent,
 float elapsed,float turn_rate,float bank_scale);
}
