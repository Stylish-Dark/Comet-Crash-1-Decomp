#include "comet/arena_ground_terrain.hpp"
#include <cassert>
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
using namespace comet::decomp;
int main(){
 ArenaRoutingGrid grid;ArenaGroundTerrainGeometryQuery q;q.cell={3,3};
 auto g=prepare_arena_ground_terrain(q,grid);assert(g && (g->limits==std::array<float,4>{0,24,0,24}));
 grid.cells[3*24+4].visibility=1;g=prepare_arena_ground_terrain(q,grid);assert(g && g->limits[1]==4);
 q.visibility_bit=1;g=prepare_arena_ground_terrain(q,grid);assert(g && g->limits[1]==24);q.visibility_bit=0;
 grid={};grid.cells[2*24+2].visibility=1;g=prepare_arena_ground_terrain(q,grid);assert(g && (g->corners[0]==std::array<float,4>{3,0,3,0}));
 grid.cells[3*24+2].visibility=1;g=prepare_arena_ground_terrain(q,grid);assert(g && (g->corners[0]==std::array<float,4>{3,0,3,0}));
 q.world_only=true;g=prepare_arena_ground_terrain(q,grid);assert(g && g->limits[0]==0 && (g->corners[0]==std::array<float,4>{}));
 q.width=0;assert(!prepare_arena_ground_terrain(q,grid));q.width=24;q.visibility_bit=4;assert(!prepare_arena_ground_terrain(q,grid));
 ArenaGroundPointQuery p;p.position={3.1f,1,3.1f,0};p.reference_position={3.05f,0,3.05f,0};p.point={3,0,3,0};p.radius=.12f;p.velocity={-1,0,-1,0};
 p.restitution_percent=140;
 auto r=resolve_arena_ground_point(p);assert(r && r->contacts==1 && r->position[0]>p.position[0] && r->position[1]==1);
 for(unsigned axis:{0u,2u}){assert(std::bit_cast<unsigned>(r->position[axis])==0x4048a17c);assert(std::bit_cast<unsigned>(r->velocity[axis])==0x3ecccccc);assert(std::bit_cast<unsigned>(r->accumulated_impulse[axis])==0x3fb33333);}
 p.reference_position={5,0,5,0};r=resolve_arena_ground_point(p);assert(r && !r->contacts && r->position==p.position);
 p.reference_position=p.point;assert(!resolve_arena_ground_point(p));p.reference_position={3.05f,0,3.05f,0};p.movable=false;r=resolve_arena_ground_point(p);assert(r && r->contacts==1 && r->position==p.position && r->velocity==p.velocity);
 ArenaGroundTerrainQuery t;t.geometry.cell={3,3};t.bounds.position={4.01f,1,3.5f,0};t.bounds.velocity={1,0,.5f,0};t.bounds.lateral_radius=.12f;t.bounds.restitution_percent=140;t.reference_position={3.95f,0,3.5f,0};grid={};grid.cells[3*24+4].visibility=1;
 r=contain_arena_ground_terrain(t,grid);assert(r && r->contacts==1 && r->position[0]<3.88f && r->velocity[0]<0 && r->velocity[2]==.5f);
 int previous=std::fegetround();std::fesetround(FE_UPWARD);r=contain_arena_ground_terrain(t,grid);assert(r && std::fegetround()==FE_UPWARD);std::fesetround(previous);
 t.reference_position[0]=std::numeric_limits<float>::quiet_NaN();assert(!contain_arena_ground_terrain(t,grid));
}
