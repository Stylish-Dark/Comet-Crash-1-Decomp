#include "comet/arena_ground_pair.hpp"
#include <cassert>
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
using namespace comet::decomp;
void bits(const std::array<float,4>&v,const std::array<unsigned,4>&e){for(unsigned i=0;i<4;i++)assert(std::bit_cast<unsigned>(v[i])==e[i]);}
int main(){
 ArenaRandom random(1);auto v=arena_ground_random_direction(random);
 bits(v,{0xbe7e9a58,0x3f35f7dc,0x3f2870bb,0});assert(random.next()==3885783);
 ArenaRandom second(42);bits(arena_ground_random_direction(second),{0x3e98b022,0x3f69445d,0x3e918553,0});assert(second.next()==96444503);
 ArenaGroundPairQuery reference;reference.position={3,0,3,0};reference.peer_position={3.15f,0,3,0};reference.own_mass=2;reference.peer_mass=3;
 ArenaRandom jitter(1);reference.jitter=arena_ground_random_direction(jitter);
 auto sample=prepare_arena_ground_pair(reference);assert(sample);
 bits(sample->position,{0x40400000,0,0x40400000,0});bits(sample->normal,{0x3f7ebf14,0x3d948e27,0x3d8982ec,0});
 assert(std::bit_cast<unsigned>(sample->gap)==0xbd4cccb7 && std::bit_cast<unsigned>(sample->response_share)==0x3f199999);
 ArenaGroundPairQuery q;q.position={3,0,3,0};q.peer_position={3.15f,0,3,0};q.jitter={0,1,0,0};
 auto p=prepare_arena_ground_pair(q);assert(p && p->overlapping && p->gap<0 && p->normal[1]>0 && std::bit_cast<unsigned>(p->response_share)==0x3effffff);
 q.peer_position=q.position;p=prepare_arena_ground_pair(q);assert(p && p->position[2]<3 && p->overlapping);
 q.index=2;p=prepare_arena_ground_pair(q);assert(p && p->position[2]>3);
 q.peer_position={5,0,3,0};p=prepare_arena_ground_pair(q);assert(p && !p->overlapping);
 q.own_mass=q.peer_mass=0;p=prepare_arena_ground_pair(q);assert(p && p->response_share==0);
 q.own_mass=-1;assert(!prepare_arena_ground_pair(q));q.own_mass=1;q.jitter={0,2,0,0};assert(!prepare_arena_ground_pair(q));
 q.jitter={0,1,0,0};q.peer_position[0]=std::numeric_limits<float>::infinity();assert(!prepare_arena_ground_pair(q));
 int prior=std::fegetround();std::fesetround(FE_UPWARD);ArenaRandom third(1);bits(arena_ground_random_direction(third),{0xbe7e9a58,0x3f35f7dc,0x3f2870bb,0});assert(std::fegetround()==FE_UPWARD);std::fesetround(prior);
}
