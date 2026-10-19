#pragma once
#include <array>
namespace comet::decomp {
struct ArenaCraftFlightState {
 std::array<float,4> position{},velocity{};
};
// FE5D8 selected free-flight arithmetic; excludes collision/action/state gates.
// mode_flag is snapshot+28D4; reduced_speed is controller byte+3E.
void advance_arena_craft_flight(ArenaCraftFlightState&,float x,float z,float elapsed,
                              bool mode_flag=true,bool reduced_speed=false);
}
