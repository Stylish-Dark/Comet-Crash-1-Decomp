#pragma once
#include <array>
#include <optional>
namespace comet::decomp {
struct ArenaGroundHeightQuery {
 std::array<float,4> position{},velocity{},direction{};
 bool enabled=false;
};
// 143A0..1441C: height error/vertical damping, scaled by four and normalized.
std::optional<std::array<float,4>> steer_arena_ground_height(const ArenaGroundHeightQuery &);
}
