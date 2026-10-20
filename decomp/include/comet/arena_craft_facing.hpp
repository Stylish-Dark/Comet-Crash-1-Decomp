#pragma once
#include <array>
namespace comet::decomp {
struct ArenaCraftFacingState {
 float yaw=0; // controller +5C
 std::array<float,4> pending_orientation{0,0,0,1}; // controller +70
};
// Selected ordinary movement-facing (controller +3F==0), not target/attachment
// facing or banking. Returns the previous quaternion displayed by 11300 while
// committing the next quaternion to pending_orientation as at 1139C.
// Input is the capped command; mode/reduced match the selected flight branches.
// Unsupported input throws without changing state. Aligned-axis math uses the
// existing weapon-aim finite analytic limits, not SPU extended-exponent parity.
std::array<float,4> advance_arena_craft_facing(ArenaCraftFacingState&,
 float x,float z,float elapsed,bool mode_flag=true,bool reduced_speed=false);
}
