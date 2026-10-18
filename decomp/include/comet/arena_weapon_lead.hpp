#pragma once
#include <array>
#include <optional>
namespace comet::decomp {
// SPU 1EEC8: constant-velocity intercept, with original quadratic fallbacks.
// Finite XYZ vectors require W=0 and absolute components <=100000.
// Projectile speed is in (0,10000]. No state is mutated.
std::optional<std::array<float,4>> predict_arena_weapon_target(
    const std::array<float,4>& origin,
    const std::array<float,4>& target_position,
    const std::array<float,4>& target_velocity, float projectile_speed);
}
