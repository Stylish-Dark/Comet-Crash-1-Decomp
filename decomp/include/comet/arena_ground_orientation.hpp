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
