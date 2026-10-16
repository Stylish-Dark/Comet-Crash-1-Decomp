#include "comet/arena_ground_contact.hpp"
#include <cassert>
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
using namespace comet::decomp;
void bits(const std::array<float,4>&v,const std::array<unsigned,4>&expected){
  for(unsigned i=0;i<4;++i)assert(std::bit_cast<unsigned>(v[i])==expected[i]);
}
int main(){
 ArenaGroundContactQuery q;q.position={.05f,0,3.5f,0};q.velocity={-1,0,0,0};q.normal={-1,0,0,0};
 q.penetration=.05f;q.restitution_percent=140;
 auto r=resolve_arena_ground_contact(q);assert(r);
 bits(r->position,{0x3dcccf6b,0,0x40600000,0});bits(r->velocity,{0x3eccccc8,0,0,0});
 bits(r->accumulated_impulse,{0x3fb33332,0,0,0});assert(!r->energetic_impact);
 // Separation still corrects penetration, but never reverses escaping velocity.
 q.velocity={1,0,0,0};r=resolve_arena_ground_contact(q);assert(r);
 bits(r->position,{0x3dcccf6b,0,0x40600000,0});bits(r->velocity,{0x3f800000,0,0,0});
 bits(r->accumulated_impulse,{0,0,0,0});
 q.velocity={-1,0,0,0};q.movable=false;r=resolve_arena_ground_contact(q);assert(r);
 bits(r->position,{0x3d4ccccd,0,0x40600000,0});bits(r->velocity,{0xbf800000,0,0,0});
 bits(r->accumulated_impulse,{0x3fb33332,0,0,0});
 q.movable=true;q.velocity={};q.response_share=.5f;q.restitution_percent=100;r=resolve_arena_ground_contact(q);assert(r);
 bits(r->position,{0x3d999c38,0,0x40600000,0});bits(r->velocity,{0,0,0,0});
 q.velocity={1,2,3,0};q.peer_velocity={-2,-3,1,0};q.normal={.6f,0,.8f,0};
 q.penetration=.07f;q.response_share=.25f;q.restitution_percent=120;r=resolve_arena_ground_contact(q);assert(r);
 bits(r->position,{0x3d21c659,0,0x405f1a88,0});bits(r->velocity,{0x3ec6a7f2,0x40000000,0x400bc6a8,0});
 bits(r->accumulated_impulse,{0xbf1cac07,0,0xbf50e55f,0});
 ArenaGroundBoundsQuery b;b.position={.05f,1,3.5f,0};b.velocity={-1,0,0,0};b.restitution_percent=140;
 auto bounded=contain_arena_ground_unit(b);assert(bounded && bounded->contacts==1);
 assert(bounded->position[0]>.1f && bounded->velocity[0]>0);
 b.position={2,1,3,0};bounded=contain_arena_ground_unit(b);assert(bounded && !bounded->contacts && bounded->position==b.position);
 b.position={-.02f,-.1f,24.02f,0};b.velocity={-2,-1,3,0};b.radius=.1f;b.lateral_radius=.12f;b.restitution_percent=140;
 bounded=contain_arena_ground_unit(b);assert(bounded && bounded->contacts==3);
 bits(bounded->position,{0x3df5c9e1,0x3dccd745,0x41bf0a36,0});
 bits(bounded->velocity,{0x3f4cccc8,0x3eccccc8,0xbf999994,0});
 bits(bounded->accumulated_impulse,{0x40333332,0x3fb33332,0xc0866665,0});
 b.position={2,.11f,3,0};b.velocity={0,-1,0,0};b.floor_uses_lateral_radius=true;
 bounded=contain_arena_ground_unit(b);assert(bounded && bounded->contacts==1);
 bits(bounded->position,{0x40000000,0x3df5c315,0x40400000,0});
 b.position={24.05f,2.1f,-.1f,0};b.velocity={3,2,-1,0};b.floor_uses_lateral_radius=false;
 bounded=contain_arena_ground_unit(b);assert(bounded && bounded->contacts==3);
 bits(bounded->position,{0x41bf0a35,0x3ff3328c,0x3df5ce13,0});
 bits(bounded->velocity,{0xbf999994,0xbf4cccc8,0x3eccccc8,0});
 b.position={2,1,3,0};b.velocity={};b.accumulated_impulse={2,3,4,0};
 bounded=contain_arena_ground_unit(b);assert(bounded && bounded->accumulated_impulse==b.accumulated_impulse);
 b.position[1]=-.1f;b.velocity[1]=-2;
 bounded=contain_arena_ground_unit(b);assert(bounded && bounded->accumulated_impulse[0]==2 && bounded->accumulated_impulse[1]>5 && bounded->accumulated_impulse[2]==4);
 b.accumulated_impulse={};
 int previous=std::fegetround();std::fesetround(FE_UPWARD);r=resolve_arena_ground_contact(q);assert(r && std::fegetround()==FE_UPWARD);std::fesetround(previous);
 q.penetration=-1;assert(!resolve_arena_ground_contact(q));q.penetration=.1f;q.normal={};assert(!resolve_arena_ground_contact(q));
 b.max_x=0;assert(!contain_arena_ground_unit(b));b.max_x=24;b.position[0]=std::numeric_limits<float>::quiet_NaN();assert(!contain_arena_ground_unit(b));
}
