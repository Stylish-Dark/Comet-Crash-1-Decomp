#include "comet/native_pilot.hpp"
#include "comet/arena_craft_flight.hpp"
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
 extent_=extent;position_={cell.x+.5f,.85f,cell.z+.5f};orientation_={0,0,0,1};stop();
}
void NativePilot::advance(float x,float z,float elapsed,bool enabled){
 if(!std::isfinite(elapsed)||elapsed<0||elapsed>.25f)throw std::invalid_argument("invalid pilot elapsed");
 const auto stick=arena_player_stick(x,z);
 if(!enabled){stop();return;}
 ArenaCraftFlightState state{{position_.x,position_.y,position_.z,0},velocity_};
 // Select original mode-flag=1 free flight. Arena clipping/heading stay preview policy.
 advance_arena_craft_flight(state,stick.x,stick.z,elapsed,true,false);
 position_={std::clamp(state.position[0],.5f,extent_-.5f),state.position[1],std::clamp(state.position[2],.5f,extent_-.5f)};
 velocity_=state.velocity;
 if(velocity_[0]!=0||velocity_[2]!=0){const float half=.5f*std::atan2(velocity_[0],velocity_[2]);orientation_={0,std::sin(half),0,std::cos(half)};}

}
ArenaGridPosition NativePilot::cursor() const{return {static_cast<std::uint8_t>(position_.x),static_cast<std::uint8_t>(position_.z)};}
}
