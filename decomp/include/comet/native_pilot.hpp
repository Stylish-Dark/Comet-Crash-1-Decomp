#pragma once
#include "comet/arena_state.hpp"
#include "comet/arena_player_craft.hpp"
#include "comet/arena_craft_facing.hpp"
namespace comet::decomp {
struct ArenaPlayerStick {float x=0,z=0,magnitude=0;};
// Semantic PPU F2DB4..F2E90 cap. Portable sqrt/divide replaces VMX estimates;
// this preserves the rule, not the original approximation's bit patterns.
ArenaPlayerStick arena_player_stick(float x,float z);
// Recovered flight/arena containment with native lifecycle and selected ordinary facing policy.
class NativePilot {
 ModelPosition position_{.5f,.85f,.5f};
 std::array<float,4> orientation_{0,0,0,1};
 ArenaCraftBounds bounds_=arena_player_craft_bounds(24,24);
 std::array<float,4> velocity_{};
 ArenaCraftFacingState facing_{};
public:
 void reset(ArenaGridPosition,unsigned extent);
 void stop(){velocity_={};}
 void advance(float x,float z,float elapsed,bool enabled);
 ModelPosition position() const{return position_;}
 const std::array<float,4>& orientation() const{return orientation_;}
 ArenaGridPosition cursor() const;
};
}
