#include "comet/arena_ground_deployment.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
bool number(float f){return std::isfinite(f) && std::fpclassify(f)!=FP_SUBNORMAL;}
struct Rounding {
 int previous=std::fegetround();
 Rounding(){if(previous<0 || std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("deployment rounding unavailable");}
 ~Rounding(){std::fesetround(previous);}
};
}
ArenaGroundDeploymentGate advance_arena_ground_deployment(
 float &timer,ArenaGroundDeploymentControl control,float elapsed){
 if(!number(timer) || timer<0 || !number(elapsed) || elapsed<0 || elapsed>10 ||
    control.kind<40 || control.kind>48 || !number(control.strength) || control.strength<0 || control.strength>1)
  return ArenaGroundDeploymentGate::Invalid;
 // Job table 20BB0: the four fast kinds use bits 3D4CCCCC, not rounded .05f.
 constexpr std::array<unsigned,9> intervals{0x3d4cccccu,0x3d4cccccu,0x3d4cccccu,0x3d4cccccu,
                                         0x3f000000u,0x3f800000u,0x3f000000u,0x3f800000u,0x3f800000u};
 Rounding rounding;
 volatile float updated=timer+elapsed;
 volatile float difference=std::bit_cast<float>(intervals[control.kind-40])-2.f;
 const float threshold=std::fma(float(difference),control.strength,2.f);
 if(!number(updated))return ArenaGroundDeploymentGate::Invalid;
 timer=updated;
 if(control.strength==0)return ArenaGroundDeploymentGate::Inactive;
 return timer>threshold?ArenaGroundDeploymentGate::Ready:ArenaGroundDeploymentGate::Waiting;
}
std::optional<std::array<float,4>> arena_ground_deployment_position(ArenaGridPosition cell){
 if(cell.x>=24 || cell.z>=24)return {};
 return std::array<float,4>{float(cell.x)+.5f,0,float(cell.z)+.5f,0};
}
}
