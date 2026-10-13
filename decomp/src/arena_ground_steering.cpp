#include "comet/arena_ground_steering.hpp"
#include "arena_spu_math.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
using Vector=std::array<float,4>;
float multiply(float a,float b){volatile float r=a*b;return r;}
float add(float a,float b){volatile float r=a+b;return r;}
float subtract(float a,float b){volatile float r=a-b;return r;}
Vector difference(const Vector&a,const Vector&b){Vector r;for(unsigned i=0;i<4;++i)r[i]=subtract(a[i],b[i]);return r;}
Vector sum(const Vector&a,const Vector&b){Vector r;for(unsigned i=0;i<4;++i)r[i]=add(a[i],b[i]);return r;}
Vector product(const Vector&a,const Vector&b){Vector r;for(unsigned i=0;i<4;++i)r[i]=multiply(a[i],b[i]);return r;}
Vector fused(const Vector&a,const Vector&b,const Vector&c){Vector r;for(unsigned i=0;i<4;++i)r[i]=std::fma(a[i],b[i],c[i]);return r;}
Vector negative_fused(const Vector&a,const Vector&b,const Vector&c){Vector r;for(unsigned i=0;i<4;++i)r[i]=std::fma(-a[i],b[i],c[i]);return r;}
float dot(const Vector&a,const Vector&b){return std::fma(a[2],b[2],std::fma(a[1],b[1],multiply(a[0],b[0])));}
Vector repeat(float v){return {v,v,v,v};}
// The original quaternion-vector product retains the exact operation order.
Vector forward(const Vector&q){
 const Vector negative_z{0,0,-1,0},negative_y{0,-1,0,0},negative_x{-1,0,0,0};
 const Vector permute_yzx{q[1],q[2],q[0],q[3]},permute_zxy{q[2],q[0],q[1],q[3]};
 const auto multiplied=product(q,negative_z);
 const auto first=fused(negative_y,{q[1],q[2],q[3],q[0]},multiplied);
 const float projection=add(first[0],multiplied[2]);
 auto cross=negative_fused(permute_zxy,negative_y,
                          fused(permute_yzx,negative_x,product(repeat(q[3]),negative_z)));
 const Vector cross_zxy{cross[2],cross[0],cross[1],cross[3]},cross_yzx{cross[1],cross[2],cross[0],cross[3]};
 auto rotated=negative_fused(permute_zxy,cross_yzx,
               fused(permute_yzx,cross_zxy,fused(repeat(q[3]),cross,product(repeat(projection),q))));
 return product(rotated,{1,0,1,0});
}
struct Rounding {
 int previous=std::fegetround();
 Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("steering rounding unavailable");}
 ~Rounding(){std::fesetround(previous);}
};
float distance(const Vector&v){
 const float squared=dot(v,v);
 if(squared==0)return 0; // finite native zero-distance policy
 const float inverse=detail::inverse_root_estimate(squared);
 const float root=multiply(inverse,squared);
 const float residual=std::fma(-inverse,root,std::bit_cast<float>(0x3f800001u));
 return std::fma(residual,multiply(root,.5f),root);
}
}
std::optional<ArenaGroundSteeringResult> steer_arena_ground_unit(
 const ArenaGroundSteeringQuery&q,const ArenaRoutingGrid&grid){
 if(q.extent==0||q.extent>24||q.owner>=4||q.visibility_bit>=32 ||
    (q.target_available&&(q.target>=4||q.target==q.owner||q.target_base.x>=q.extent||q.target_base.z>=q.extent)))return {};
 for(float f:q.position)if(!std::isfinite(f))return {};
 for(float f:q.orientation)if(!std::isfinite(f)||std::abs(f)>2)return {};
 Rounding rounding;
 ArenaGroundSteeringResult result;result.blocked=q.initial_blocked;
 if(!q.target_available){result.direction=forward(q.orientation);return result;}
 Vector position;
 for(unsigned i=0;i<4;++i)position[i]=std::clamp(q.position[i],0.f,subtract(float(q.extent),.5f));
 const int x=int(std::trunc(position[0])),z=int(std::trunc(position[2]));
 const Vector center{add(float(x),.5f),0,add(float(z),.5f),0};
 if(x==q.target_base.x&&z==q.target_base.z){result.direction=difference(center,position);result.blocked=false;return result;}
 const unsigned pair=3*q.owner+q.target-(q.target>q.owner?1:0);
 const auto route=grid.cells[z*24+x].routes[pair];
 auto fixed=[](unsigned direction){auto v=*arena_ground_route_vector(static_cast<std::uint8_t>(direction));return Vector{v[0],v[1],v[2],0};};
 if(q.direct_route){result.direction=fixed((route>>1)&7);result.route_direction=(route>>1)&7;result.blocked=false;return result;}
 if(route&1){result.direction=forward(q.orientation);result.blocked=true;return result;}
 const unsigned primary=(route>>1)&7;
 const auto primary_vector=fixed(primary);
 result.direction=primary_vector;result.route_direction=primary;result.blocked=false;
 float best=distance(difference(sum(center,primary_vector),position));
 // Exact original candidate order: west, east, negative z, positive z.
 constexpr std::array<std::array<int,2>,4> offsets{{{-1,0},{1,0},{0,-1},{0,1}}};
 constexpr std::array<unsigned,4> direction_ids{3,7,5,1};
 for(unsigned n=0;n<4;++n){
   const int cx=std::clamp(x+offsets[n][0],0,int(q.extent)-1);
   const int cz=std::clamp(z+offsets[n][1],0,int(q.extent)-1);
   const auto &candidate=grid.cells[cz*24+cx];
   const auto step_vector=fixed(direction_ids[n]);
   const auto displacement=difference(sum(center,step_vector),position);
   const auto candidate_direction=(candidate.visibility>>q.visibility_bit)&1
       ? primary_vector:fixed((candidate.routes[pair]>>1)&7);
   const bool suppressed=cx==q.suppressed_cell[0]&&cz==q.suppressed_cell[1];
   const float length=distance(displacement);
   if(!suppressed && dot(candidate_direction,step_vector)>std::bit_cast<float>(0xb8d1b717u) &&
      dot(primary_vector,candidate_direction)<.5f && best>length){
     best=length;result.direction=step_vector;result.route_direction=direction_ids[n];
   }
 }
 return result;
}
} // namespace comet::decomp
