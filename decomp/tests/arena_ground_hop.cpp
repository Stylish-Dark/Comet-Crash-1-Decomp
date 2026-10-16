#include "comet/arena_ground_hop.hpp"
#include <cassert>
#include <cmath>
#include <limits>
using namespace comet::decomp;
int main(){
 ArenaGroundHopQuery q;q.elapsed=.05f;q.speed_limit=2;q.steering=q.lookahead_steering={1,0,0,0};q.grounded=true;q.flags=2;
 ArenaRandom random(17890);auto result=advance_arena_ground_hop(q,random);
 assert(result&&result->hopped&&result->timer>3.9f&&result->timer<4&&result->position_y&&*result->position_y<0);
 assert(result->velocity[1]>1 && result->velocity[0]>1);
 q.timer=.05f;ArenaRandom a(2),b(2);result=advance_arena_ground_hop(q,a);assert(result&&!result->hopped&&!result->position_y&&result->timer==0);assert(a.next()==b.next());
 q.timer=1;q.grounded=false;q.velocity={0,-1,0,0};result=advance_arena_ground_hop(q,a);assert(result&&result->acceleration[1]<-7);
 q.velocity={0,0,0,0};result=advance_arena_ground_hop(q,a);assert(result&&result->acceleration[0]==0);
 q.timer=0;q.steering=q.lookahead_steering={0,0,0,0};auto before=a;assert(!advance_arena_ground_hop(q,a));assert(a.next()==before.next());
 q.elapsed=std::numeric_limits<float>::infinity();assert(!advance_arena_ground_hop(q,a));
}
