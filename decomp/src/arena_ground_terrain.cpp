#include "comet/arena_ground_terrain.hpp"
#include "arena_spu_math.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
using Vector=std::array<float,4>;
bool finite(const Vector &v){for(float f:v)if(!std::isfinite(f))return false;return true;}
float mul(float a,float b){volatile float r=a*b;return r;}
float sub(float a,float b){volatile float r=a-b;return r;}
struct Rounding {
 int previous=std::fegetround();
 Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("terrain rounding unavailable");}
 ~Rounding(){std::fesetround(previous);}
};
}
std::optional<ArenaGroundTerrainGeometry> prepare_arena_ground_terrain(
 const ArenaGroundTerrainGeometryQuery&q,const ArenaRoutingGrid&grid){
 if(!q.width||!q.height||q.width>24||q.height>24||q.cell.x>=q.width||q.cell.z>=q.height||q.visibility_bit>=4)return {};
 ArenaGroundTerrainGeometry r;r.limits={0,float(q.width),0,float(q.height)};
 if(q.world_only)return r;
 const int x=q.cell.x,z=q.cell.z;
 auto blocked=[&](int px,int pz){return px>=0&&pz>=0&&px<q.width&&pz<q.height&&(grid.cells[pz*24+px].visibility&(1u<<q.visibility_bit));};
 const bool west=blocked(x-1,z),east=blocked(x+1,z),north=blocked(x,z-1),south=blocked(x,z+1);
 if(west)r.limits[0]=float(x);
 if(east)r.limits[1]=float(x+1);
 if(north)r.limits[2]=float(z);
 if(south)r.limits[3]=float(z+1);
 if(x==0||z==0||blocked(x-1,z-1))r.corners[0]={float(x),0,float(z),0};
 if(x==0||z==q.height-1||blocked(x-1,z+1))r.corners[1]={float(x),0,float(z+1),0};
 if(x==q.width-1||z==0||blocked(x+1,z-1))r.corners[2]={float(x+1),0,float(z),0};
 if(x==q.width-1||z==q.height-1||blocked(x+1,z+1))r.corners[3]={float(x+1),0,float(z+1),0};
 return r;
}
std::optional<ArenaGroundBoundsResult> resolve_arena_ground_point(const ArenaGroundPointQuery&q){
 if(!finite(q.position)||!finite(q.velocity)||!finite(q.reference_position)||!finite(q.point)||!finite(q.accumulated_impulse)||!std::isfinite(q.radius)||q.radius<0)return {};
 Rounding rounding;
 Vector delta;for(unsigned i=0;i<4;i++)delta[i]=sub(q.point[i],q.reference_position[i]);
 const float squared=std::fma(delta[2],delta[2],std::fma(delta[1],delta[1],mul(delta[0],delta[0])));
 // Exact coincident point needs an undefined normal; reject transactionally.
 if(!std::isnormal(squared))return {};
 const float estimate=detail::inverse_root_estimate(squared),root=mul(estimate,squared);
 const float distance=std::fma(std::fma(-estimate,root,std::bit_cast<float>(0x3f800001u)),mul(root,.5f),root);
 const float gap=sub(distance,q.radius);
 if(!std::isnormal(distance)||!std::isfinite(gap))return {};
 ArenaGroundBoundsResult r;r.position=q.position;r.velocity=q.velocity;r.accumulated_impulse=q.accumulated_impulse;
 if(!(std::bit_cast<float>(0xb8d1b717u)>gap))return r;
 const float inverse=detail::reciprocal_estimate(distance);
 const float residual=std::fma(-distance,inverse,std::bit_cast<float>(0x3f800001u));
 ArenaGroundContactQuery c;c.position=q.position;c.velocity=q.velocity;c.accumulated_impulse=q.accumulated_impulse;
 for(unsigned i=0;i<4;i++){const float base=mul(delta[i],inverse);c.normal[i]=std::fma(residual,base,base);}
 c.penetration=-gap;c.restitution_percent=q.restitution_percent;c.movable=q.movable;
 const auto response=resolve_arena_ground_contact(c);if(!response)return {};
 static_cast<ArenaGroundContactResult&>(r)=*response;r.contacts=1;return r;
}
std::optional<ArenaGroundBoundsResult> contain_arena_ground_terrain(
 const ArenaGroundTerrainQuery&q,const ArenaRoutingGrid&grid){
 if(!finite(q.reference_position))return {};
 const auto geometry=prepare_arena_ground_terrain(q.geometry,grid);if(!geometry)return {};
 auto bounds=q.bounds;bounds.min_x=geometry->limits[0];bounds.max_x=geometry->limits[1];
 bounds.min_z=geometry->limits[2];bounds.max_z=geometry->limits[3];
 auto result=contain_arena_ground_unit(bounds);if(!result)return {};
 if(!q.geometry.world_only)for(const auto &point:geometry->corners){
   // Preserve original zero placeholders: they are still tested at the origin.
   ArenaGroundPointQuery c;c.position=result->position;c.velocity=result->velocity;
   c.accumulated_impulse=result->accumulated_impulse;c.reference_position=q.reference_position;c.point=point;
   c.radius=q.bounds.lateral_radius;c.restitution_percent=q.bounds.restitution_percent;c.movable=q.bounds.movable;
   const auto next=resolve_arena_ground_point(c);if(!next)return {};
   const auto count=result->contacts;const bool energetic=result->energetic_impact;
   *result=*next;result->contacts+=count;result->energetic_impact|=energetic;
 }
 return result;
}
}
