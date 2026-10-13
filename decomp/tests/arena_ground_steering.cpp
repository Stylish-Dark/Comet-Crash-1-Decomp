#include "comet/arena_ground_steering.hpp"
#include <cassert>
#include <bit>
#include <cfenv>
#include <limits>
using namespace comet::decomp;
int main() {
 ArenaRoutingGrid grid;
 ArenaGroundSteeringQuery q;
 q.position={2.25f,.25f,3.75f,0};q.target_available=true;
 q.owner=0;q.target=1;q.target_base={2,3};
 auto result=steer_arena_ground_unit(q,grid);
 assert(result && !result->blocked);
 assert((result->direction==std::array<float,4>{.25f,-.25f,-.25f,0}));
 q.target_base={20,20};q.direct_route=true;
 for(auto &c:grid.cells)c.routes.fill(14);
 result=steer_arena_ground_unit(q,grid);
 assert(result && result->route_direction==7 && result->direction[0]==1 && result->direction[2]==0);
 q.target_available=false;
 result=steer_arena_ground_unit(q,grid);
 assert(result && !result->blocked && result->direction[0]==0 && result->direction[2]==-1);
 q.target_available=true;
 grid.cells[3*24+2].routes[0]=1;
 q.direct_route=false;
 result=steer_arena_ground_unit(q,grid);
 assert(result && result->blocked && result->direction[2]==-1);
 q.target=q.owner;assert(!steer_arena_ground_unit(q,grid));
 q.target=1;q.position[0]=std::numeric_limits<float>::quiet_NaN();assert(!steer_arena_ground_unit(q,grid));
 q.position={2.9f,.23f,2.2f,0};q.target_base={20,20};q.direct_route=false;
 for(auto &c:grid.cells){c.routes.fill(0);c.visibility=0;}
 grid.cells[2*24+3].routes[0]=12; // east neighbor bends away from the current NE route
 result=steer_arena_ground_unit(q,grid);
 assert(result && result->route_direction==7 && result->direction[0]==1 && result->direction[2]==0);
 q.suppressed_cell={3,2};
 result=steer_arena_ground_unit(q,grid);
 assert(result && result->route_direction==0); // original auxiliary exclusion
 q.suppressed_cell={-1,-1};grid.cells[2*24+3].visibility=1;
 result=steer_arena_ground_unit(q,grid);
 assert(result && result->route_direction==0); // visible neighbor substitutes primary direction
 for(unsigned direction=0;direction<8;++direction){
   q.direct_route=true;for(auto &c:grid.cells)c.routes.fill(static_cast<std::uint8_t>(direction*2+1));
   result=steer_arena_ground_unit(q,grid);auto v=arena_ground_route_vector(direction);
   assert(result && v && result->direction[0]==(*v)[0] && result->direction[2]==(*v)[2] && !result->blocked);
   assert(arena_ground_route_heading(direction));
 }
 assert(!arena_ground_route_heading(8) && !arena_ground_route_vector(8));
 q.target_available=false;q.initial_blocked=true;
 result=steer_arena_ground_unit(q,grid);assert(result && result->blocked);
 // Public numeric output references from the untouched original routine.
 constexpr std::array<std::array<unsigned,4>,8> reference{{
  {0x3f800000,0,0,0},{0x3f3504f3,0,0x3f3504f3,0},
  {0x3f3504f3,0,0x3f3504f3,0},{0x3f800000,0,0,0},
  {0x3f3851ea,0,0xbe88f5bf,0x80000000},{0x3f3851ea,0,0xbe88f5bf,0x80000000},
  {0xbeccccd0,0xbe6b851f,0x3e999998,0},{0xbf3504f3,0,0xbf3504f3,0}}};
 for(unsigned n=0;n<reference.size();++n){
   ArenaGroundSteeringQuery ref;ref.position={2.9f,.23f,2.2f,0};ref.orientation={.15f,-.6f,.4f,.7f};
   ref.target_available=true;ref.target=1;ref.target_base={20,20};
   ArenaRoutingGrid ref_grid;ref_grid.cells[2*24+3].routes[0]=12;
   if(n==1)ref.suppressed_cell={3,2};
   if(n==2)ref_grid.cells[2*24+3].visibility=1;
   if(n==3){ref.direct_route=true;ref_grid.cells[2*24+2].routes[0]=15;}
   if(n==4)ref_grid.cells[2*24+2].routes[0]=1;
   if(n==5){ref.target_available=false;ref.initial_blocked=true;}
   if(n==6)ref.target_base={2,2};
   if(n==7){ref.position={-.3f,.23f,25.3f,.1f};ref.direct_route=true;ref_grid.cells[23*24].routes[0]=8;}
   auto actual=steer_arena_ground_unit(ref,ref_grid);assert(actual && actual->blocked==(n==4||n==5));
   for(unsigned axis=0;axis<4;++axis)assert(std::bit_cast<unsigned>(actual->direction[axis])==reference[n][axis]);
 }
 const auto rounding=std::fegetround();std::fesetround(FE_UPWARD);
 q.position[0]=2.25f;steer_arena_ground_unit(q,grid);
 assert(std::fegetround()==FE_UPWARD);std::fesetround(rounding);
}
