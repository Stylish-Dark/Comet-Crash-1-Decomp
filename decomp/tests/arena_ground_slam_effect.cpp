#include "comet/arena_ground_slam_effect.hpp"
#include "comet/arena_unit_entity.hpp"
#include <cassert>
#include <cmath>
#include <limits>
#include <memory>
using namespace comet::decomp;
int main(){
 auto world=std::make_unique<ArenaEntityStorage>();for(auto &b:world->banks)b.resize(5);
 reset_arena_entity_index_pool(*world,5);ArenaRandom rng{4};
 auto add=[&](unsigned kind,unsigned owner,std::array<float,4> p){
  ArenaEntitySlot slot{};assert(initialize_arena_unit_template(slot,kind,owner,p,rng));
  return insert_arena_entity(*world,{static_cast<std::uint8_t>(p[0]),static_cast<std::uint8_t>(p[2])},slot);
 };
 assert(add(44,0,{3.5f,.2f,3.5f,0})==0);
 assert(add(40,1,{3.7f,.2f,3.5f,0})==1);
 assert(add(40,0,{3.6f,.2f,3.5f,0})==2);
 assert(add(40,1,{4.8f,.2f,3.5f,0})==3);
 assert(add(40,1,{3.9f,.2f,3.5f,0})==4);
 std::array<float,5> health{};auto banks=world->banks;auto cells=world->cells;
 auto hit=accumulate_arena_ground_slam(*world,0,2,{0,2,0,0},health);
 assert(hit && *hit==std::vector<std::uint16_t>({1,4}));
 assert(health[1]<-4.8f && health[1]>=-5 && health[4]<0 && health[4]>health[1]);
 assert(health[0]==0 && health[2]==0 && health[3]==0);
 assert(world->banks==banks && world->cells==cells);
 auto before=health;
 hit=accumulate_arena_ground_slam(*world,0,2,{0,1,0,0},health);
 assert(hit && hit->empty() && health==before);
 assert(!accumulate_arena_ground_slam(*world,1,2,{0,2,0,0},health));assert(health==before);
 assert(!accumulate_arena_ground_slam(*world,0,2,{0,std::numeric_limits<float>::infinity(),0,0},health));assert(health==before);
 // All deltas commit together; invalid later recipient must not retain an earlier hit.
 health[4]=std::numeric_limits<float>::quiet_NaN();const float first=health[1];
 assert(!accumulate_arena_ground_slam(*world,0,2,{0,2,0,0},health));assert(health[1]==first && std::isnan(health[4]));
 health={};world->cells[0][3*24+3][1][3]=2;world->cells[1]=world->cells[0];
 hit=accumulate_arena_ground_slam(*world,0,2,{0,2,0,0},health);
 assert(hit && *hit==std::vector<std::uint16_t>({4}) && health[1]==0);
}
