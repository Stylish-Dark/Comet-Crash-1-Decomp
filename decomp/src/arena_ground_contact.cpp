#include "comet/arena_ground_contact.hpp"
#include "arena_spu_math.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
using Vector=std::array<float,4>;
float multiply(float a,float b){volatile float v=a*b;return v;}
float add(float a,float b){volatile float v=a+b;return v;}
float subtract(float a,float b){volatile float v=a-b;return v;}
float dot(const Vector&a,const Vector&b){return std::fma(a[2],b[2],std::fma(a[1],b[1],multiply(a[0],b[0])));}
bool finite(const Vector&v){for(float f:v)if(!std::isfinite(f))return false;return true;}
struct Rounding {
 int previous=std::fegetround();
 Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("contact rounding unavailable");}
 ~Rounding(){std::fesetround(previous);}
};
float inverse_length(float squared){
 const float estimate=detail::inverse_root_estimate(squared);
 const float root=multiply(estimate,squared);
 const float half=multiply(estimate,.5f);
 const float residual=std::fma(-estimate,root,std::bit_cast<float>(0x3f800001u));
 return std::fma(residual,half,estimate);
}
Vector normalized(const Vector&v,float squared){
 if(squared==0)return v;
 const float inverse=inverse_length(squared);
 Vector result;for(unsigned i=0;i<4;++i)result[i]=multiply(v[i],inverse);
 return result;
}
ArenaGroundContactResult contact(const ArenaGroundContactQuery&q){
 ArenaGroundContactResult result{q.position,q.velocity,q.accumulated_impulse,false};
 const float own_squared=dot(q.velocity,q.velocity),peer_squared=dot(q.peer_velocity,q.peer_velocity);
 const auto peer_direction=normalized(q.peer_velocity,peer_squared);
 const auto own_direction=normalized(q.velocity,own_squared);
 const float epsilon=std::bit_cast<float>(0x38d1b717u);
 const float threshold=std::bit_cast<float>(0x3e4cccccu);
 const bool separating=(peer_squared>add(own_squared,epsilon)&&dot(peer_direction,q.normal)>threshold)||
     (own_squared>add(peer_squared,epsilon)&&dot(own_direction,q.normal)<-threshold);
 if(!separating){
   Vector relative;for(unsigned i=0;i<4;++i)relative[i]=subtract(q.peer_velocity[i],q.velocity[i]);
   const float projection=dot(relative,q.normal);
   const float restitution=multiply(float(q.restitution_percent),std::bit_cast<float>(0x3c23d70au));
   const float factor=multiply(q.response_share,restitution);
   Vector impulse;
   for(unsigned i=0;i<4;++i){
     impulse[i]=multiply(multiply(q.normal[i],projection),factor);
     result.accumulated_impulse[i]=add(q.accumulated_impulse[i],impulse[i]);
     if(q.movable)result.velocity[i]=add(q.velocity[i],impulse[i]);
   }
   const float squared=dot(impulse,impulse);
   if(squared>0){
     const float estimate=detail::inverse_root_estimate(squared);
     const float root=multiply(estimate,squared);
     const float residual=std::fma(-estimate,root,std::bit_cast<float>(0x3f800001u));
     const float length=std::fma(residual,multiply(root,.5f),root);
     result.energetic_impact=length>2.f;
   }
 }
 // 7938: original movable-bit positional correction precedes effect handling.
 if(q.movable){
   const float correction=multiply(add(q.response_share,epsilon),-q.penetration);
   for(unsigned i=0;i<4;++i)result.position[i]=std::fma(q.normal[i],correction,q.position[i]);
 }
 return result;
}
}
std::optional<ArenaGroundContactResult> resolve_arena_ground_contact(const ArenaGroundContactQuery&q){
 if(!finite(q.position)||!finite(q.velocity)||!finite(q.peer_velocity)||!finite(q.normal)||!finite(q.accumulated_impulse)||
    !std::isfinite(q.penetration)||q.penetration<0 ||
    !std::isfinite(q.response_share)||q.response_share<0||q.response_share>1)return {};
 Rounding rounding;
 const float n=dot(q.normal,q.normal),own=dot(q.velocity,q.velocity),peer=dot(q.peer_velocity,q.peer_velocity);
 if(!std::isnormal(n)||std::abs(n-1.f)>.01f||(!std::isnormal(own)&&own!=0)||(!std::isnormal(peer)&&peer!=0))return {};
 auto result=contact(q);
 if(!finite(result.position)||!finite(result.velocity)||!finite(result.accumulated_impulse))return {};
 return result;
}
std::optional<ArenaGroundBoundsResult> contain_arena_ground_unit(const ArenaGroundBoundsQuery&q){
 for(float f:{q.min_x,q.max_x,q.min_z,q.max_z,q.min_y,q.max_y,q.radius,q.lateral_radius})
   if(!std::isfinite(f))return {};
 if(!finite(q.position)||!finite(q.velocity)||q.radius<0||q.lateral_radius<0||
    q.max_x<=q.min_x||q.max_z<=q.min_z||q.max_y<=q.min_y||
    q.max_x-q.min_x<2*q.lateral_radius||q.max_z-q.min_z<2*q.lateral_radius||
    q.max_y-q.min_y<q.radius+(q.floor_uses_lateral_radius?q.lateral_radius:q.radius))return {};
 Rounding rounding;
 ArenaGroundBoundsResult result;result.position=q.position;result.velocity=q.velocity;
 auto plane=[&](unsigned axis,float boundary,bool upper,float radius)->bool{
   // Preserve the original two subtractions before testing zero > difference.
   const float difference=upper?subtract(boundary,add(result.position[axis],radius)):
       subtract(subtract(result.position[axis],radius),boundary);
   if(difference>=0)return true;
   ArenaGroundContactQuery c;c.position=result.position;c.velocity=result.velocity;
   c.accumulated_impulse=result.accumulated_impulse;
   c.normal[axis]=upper?1.f:-1.f;c.penetration=-difference;
   c.restitution_percent=q.restitution_percent;c.movable=q.movable;
   const auto response=resolve_arena_ground_contact(c);if(!response)return false;
   result.position=response->position;result.velocity=response->velocity;result.accumulated_impulse=response->accumulated_impulse;
   result.energetic_impact|=response->energetic_impact;++result.contacts;return true;
 };
 if(!plane(0,q.min_x,false,q.lateral_radius)||!plane(0,q.max_x,true,q.lateral_radius)||
    !plane(2,q.min_z,false,q.lateral_radius)||!plane(2,q.max_z,true,q.lateral_radius)||
    !plane(1,q.min_y,false,q.floor_uses_lateral_radius?q.lateral_radius:q.radius)||
    !plane(1,q.max_y,true,q.radius))return {};
 return result;
}
} // namespace comet::decomp
