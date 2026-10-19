#pragma once
#include <array>
#include <cstdint>
namespace comet::decomp {
struct ArenaCraftFlightState {
 std::array<float,4> position{},velocity{};
};
// FE5D8 selected free-flight arithmetic; excludes collision/action/state gates.
// mode_flag is snapshot+28D4; reduced_speed is controller byte+3E.
void advance_arena_craft_flight(ArenaCraftFlightState&,float x,float z,float elapsed,
                              bool mode_flag=true,bool reduced_speed=false);
// 7DD0 supplies integer arena extents, floor 0, ceiling 2, no sphere points.
// Radius/material defaults are native preview choices, not craft ctor recovery.
struct ArenaCraftBounds {
 unsigned width=24,depth=24;
 float radius=.5f,lateral_radius=.5f;
 std::uint8_t restitution_percent=100;
 bool movable=true,floor_uses_lateral_radius=false;
};
struct ArenaCraftBoundaryContact {
 std::array<float,4> accumulated_impulse{};
 std::uint8_t contacts=0;
 bool energetic_impact=false; // motion metadata, never a damage/effect event
};
// Selected ordinary flight only: integrate -> 7DD0/7A50 -> velocity response.
// Invalid input or arithmetic overflow throws without changing state.
ArenaCraftBoundaryContact advance_arena_craft_in_arena(
 ArenaCraftFlightState&,float x,float z,float elapsed,const ArenaCraftBounds&,
 bool mode_flag=true,bool reduced_speed=false);
}
