#pragma once
#include <array>
#include <optional>
namespace comet::decomp {
// FE5D8 kind-44 branch 172D0..17374. The caller supplies the current tick's
// complete contact impulse (context +4D0), with W=0. No state is mutated.
std::optional<bool> arena_ground_slam_ready(const std::array<float,4> &accumulated_impulse);
}
