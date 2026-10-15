#pragma once
#include "comet/native_construction_scene.hpp"
#include <array>
#include <string>
namespace comet::decomp {
// Native viewer input policy. Original deployment timing remains in simulation.
class NativeDeploymentInput {
 NativeConstructionScene &scene_;
 std::array<std::uint8_t,4> kinds_{40,40,40,40};
 std::size_t owner_=0;
 bool held_=false;
public:
 explicit NativeDeploymentInput(NativeConstructionScene &);
 ~NativeDeploymentInput();
 NativeDeploymentInput(const NativeDeploymentInput &)=delete;
 NativeDeploymentInput &operator=(const NativeDeploymentInput &)=delete;
 std::size_t owner() const { return owner_; }
 std::uint8_t kind() const { return kinds_[owner_]; }
 bool held() const { return held_; }
 void cycle_kind(int direction);
 void next_owner();
 void press();
 void release();
 void cancel() { release(); }
 std::array<std::string,5> status_lines(bool playing) const;
};
}
