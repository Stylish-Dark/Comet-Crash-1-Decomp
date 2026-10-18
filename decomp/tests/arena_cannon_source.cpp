#include "comet/arena_cannon_source.hpp"
#include <bit>
#include <cassert>
#include <limits>
using namespace comet::decomp;
static void w(ArenaEntitySlot&s,unsigned o,unsigned v){for(unsigned i=0;i<4;i++)s[o+i]=v>>(24-8*i);}
static unsigned w(const ArenaEntitySlot&s,unsigned o){unsigned v=0;for(unsigned i=0;i<4;i++)v=(v<<8)|s[o+i];return v;}
static void f(ArenaEntitySlot&s,unsigned o,float v){w(s,o,std::bit_cast<unsigned>(v));}
static ArenaEntitySlot tower(){ArenaEntitySlot s{};s[15]=21;s[0x33]=16;s[0x60]=1;s[0x1e]=s[0x1f]=255;f(s,0x78,1);f(s,0x5c,20);f(s,0xb8,.1f);f(s,0xbc,1);f(s,0xc0,1);f(s,0xd4,4);f(s,0xd8,4);s[0xdd]=100;f(s,0xe8,1);f(s,0xec,-1);s[0xcd]=10;return s;}
int main(){
 auto s=tower();ArenaCannonSourceQuery q;q.elapsed=.05f;q.gathered_target=4;q.owner_base_center={0,0,5,0};auto r=advance_arena_cannon_source(s,q);assert(r&&r->gather_target&&!r->fire&&s[0x1f]==4);assert(w(s,0xc0)==0);
 s=tower();s[0x1e]=0;s[0x1f]=4;q.target=ArenaCannonTarget{4,{0,.39f,-2,0},{},0};q.gathered_target={};r=advance_arena_cannon_source(s,q);assert(r&&r->fire);assert(w(s,0xbc)==0);
 ArenaEntitySlot p;p.fill(0xa5);assert(initialize_arena_cannon_projectile(p,s,*r->fire));assert(p[15]==3&&p[0x6f]==27&&p[0x32]==6&&w(p,0x80)==0&&w(p,0x84)==45&&p[0x88]==0xa5);assert(w(p,0x38)==0x42480000&&w(p,0x58)==0x3f800000);
 assert(s[0x1e]==255 && s[0x1f]==255); // Reacquisition cleared the cached link.
 s=tower();s[0x1e]=0;s[0x1f]=4;q.gathered_target=4;
 q.target->velocity[0]=1;r=advance_arena_cannon_source(s,q);assert(r);auto old=s;q.elapsed=std::numeric_limits<float>::infinity();assert(!advance_arena_cannon_source(s,q)&&s==old);
 s=tower();q={};q.elapsed=.05f;q.owner_base_center={};
 assert(advance_arena_cannon_source(s,q)); // Idle cannon at its own base has zero fallback direction.

}
