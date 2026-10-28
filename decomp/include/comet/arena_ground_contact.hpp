#pragma once
#include <array>
#include <cstdint>
#include <optional>
namespace comet::decomp {
// Motion-only 7510 contact response: lifecycle/effect callbacks are suppressed.
// Proven finite-normal arithmetic plus zero lanes; not SPU denormal parity.
struct ArenaGroundContactQuery {
  std::array<float,4> position{},velocity{},peer_velocity{},normal{},accumulated_impulse{};
  // Normal must be a unit vector within 0.01 in squared length.
  float penetration = 0, response_share = 1;
  std::uint8_t restitution_percent = 100;
  bool movable = true; // original entity flags bit 0x4000
};
struct ArenaGroundContactResult {
  std::array<float,4> position{},velocity{},accumulated_impulse{};
  bool velocity_response = false; // original7510 reaches77A0
  float impact_strength = 0; // source clamp((impulse length-2)*.5,0,1)
  bool energetic_impact = false; // original return value, not a damage event
};
std::optional<ArenaGroundContactResult>
resolve_arena_ground_contact(const ArenaGroundContactQuery &);
struct ArenaGroundBoundsQuery {
  std::array<float,4> position{},velocity{},accumulated_impulse{};
  float min_x=0,max_x=24,min_z=0,max_z=24,min_y=0,max_y=2;
  float radius=.1f,lateral_radius=.1f;
  std::uint8_t restitution_percent=100;
  bool movable=true, floor_uses_lateral_radius=false;
};
struct ArenaGroundBoundsResult : ArenaGroundContactResult { std::uint8_t contacts=0; };
// Six planes of 7A50, in original X-min/X-max/Z-min/Z-max/Y-min/Y-max order.
// Optional four sphere points are a separate boundary.
std::optional<ArenaGroundBoundsResult>
contain_arena_ground_unit(const ArenaGroundBoundsQuery &);
} // namespace comet::decomp
