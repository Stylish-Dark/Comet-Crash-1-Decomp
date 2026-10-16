#include "comet/arena_ground_slam.hpp"
#include <cassert>
#include <cfenv>
#include <limits>
using namespace comet::decomp;
int main(){
 auto ready=arena_ground_slam_ready({0,2,0,0});assert(ready && *ready);
 for(auto impulse:{std::array<float,4>{0,0,0,0},{0,1,0,0},{0,-2,0,0},{2,0,0,0},{1,2,0,0}}){ready=arena_ground_slam_ready(impulse);assert(ready && !*ready);}
 ready=arena_ground_slam_ready({0,1.00001f,0,0});assert(ready && *ready);
 ready=arena_ground_slam_ready({.8f,2,0,0});assert(ready && *ready);
 assert(!arena_ground_slam_ready({0,2,0,1}));
 assert(!arena_ground_slam_ready({0,std::numeric_limits<float>::infinity(),0,0}));
 assert(!arena_ground_slam_ready({0,std::numeric_limits<float>::quiet_NaN(),0,0}));
 assert(!arena_ground_slam_ready({std::numeric_limits<float>::max(),2,0,0}));
 int previous=fegetround();fesetround(FE_UPWARD);assert(arena_ground_slam_ready({0,2,0,0}));assert(fegetround()==FE_UPWARD);fesetround(previous);
}
