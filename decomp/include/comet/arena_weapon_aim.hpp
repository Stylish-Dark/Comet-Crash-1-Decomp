#pragma once
#include <array>
#include <optional>
namespace comet::decomp {
struct ArenaWeaponAimQuery {
  std::array<float,4> direction{}; // Unit desired direction; zero preserves angles.
  // Angles and nonnegative maximum steps are radians, bounded by pi.
  float yaw=0, pitch=0, yaw_step=0, pitch_step=0;
  float pitch_min=0, pitch_max=0;
};
struct ArenaWeaponAimResult {
  float yaw=0, pitch=0;
  // Source is yaw only; muzzle includes pitch. Local forward is negative Z.
  std::array<float,4> source_orientation{}, muzzle_orientation{};
  float remaining_angle=0;
};
// SPU 1A118. Bounded finite tracking. Near-vertical directions preserve angles.
// Analytic zero-root limits keep exactly horizontal/aligned cases finite.
std::optional<ArenaWeaponAimResult> prepare_arena_weapon_aim(const ArenaWeaponAimQuery &);
}
