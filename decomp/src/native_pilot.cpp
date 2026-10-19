#include "comet/native_pilot.hpp"
#include "comet/arena_craft_flight.hpp"
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
 bounds_=arena_player_craft_bounds(extent,extent);position_={cell.x+.5f,.85f,cell.z+.5f};orientation_={0,0,0,1};facing_={};stop();
}
void NativePilot::advance(float x,float z,float elapsed,bool enabled){
 if(!std::isfinite(elapsed)||elapsed<0||elapsed>.25f)throw std::invalid_argument("invalid pilot elapsed");
 const auto stick=arena_player_stick(x,z);
 if(!enabled){stop();return;}
 ArenaCraftFlightState state{{position_.x,position_.y,position_.z,0},velocity_};
 // Original constructor radii/material; native selects ordinary movement-facing.
 advance_arena_craft_in_arena(state,stick.x,stick.z,elapsed,bounds_,true,false);
 auto facing=facing_;const auto visible=advance_arena_craft_facing(facing,stick.x,stick.z,elapsed,true,false);
 position_={state.position[0],state.position[1],state.position[2]};
 velocity_=state.velocity;
 facing_=facing;orientation_=visible;

}
ArenaGridPosition NativePilot::cursor() const{return {static_cast<std::uint8_t>(position_.x),static_cast<std::uint8_t>(position_.z)};}
}
