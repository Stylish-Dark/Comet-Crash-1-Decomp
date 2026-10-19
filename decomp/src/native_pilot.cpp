#include "comet/native_pilot.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
ArenaPlayerStick arena_player_stick(float x,float z){
 if(!std::isfinite(x)||!std::isfinite(z)||std::abs(x)>1||std::abs(z)>1)
  throw std::invalid_argument("invalid player stick");
 const float magnitude=std::hypot(x,z);
 return magnitude>1?ArenaPlayerStick{x/magnitude,z/magnitude,magnitude}:ArenaPlayerStick{x,z,magnitude};
}
void NativePilot::reset(ArenaGridPosition cell,unsigned extent){
 if(extent<1||extent>24||cell.x>=extent||cell.z>=extent)throw std::invalid_argument("invalid pilot start");
 extent_=extent;position_={cell.x+.5f,.85f,cell.z+.5f};orientation_={0,0,0,1};
}
void NativePilot::advance(float x,float z,float elapsed,bool enabled){
 if(!std::isfinite(elapsed)||elapsed<0||elapsed>.25f)throw std::invalid_argument("invalid pilot elapsed");
 const auto stick=arena_player_stick(x,z);
 if(!enabled)return;
 // Native preview policy: six grid units per second, no collision/acceleration.
 position_.x=std::clamp(position_.x+stick.x*6*elapsed,.5f,extent_-.5f);
 position_.z=std::clamp(position_.z+stick.z*6*elapsed,.5f,extent_-.5f);
 if(stick.magnitude>0){const float half=.5f*std::atan2(stick.x,stick.z);orientation_={0,std::sin(half),0,std::cos(half)};}
}
ArenaGridPosition NativePilot::cursor() const{return {static_cast<std::uint8_t>(position_.x),static_cast<std::uint8_t>(position_.z)};}
}
