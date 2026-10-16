#include "comet/arena_tesla_source.hpp"
#include <cassert>
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
using namespace comet::decomp;
static void word(ArenaEntitySlot&s,unsigned o,unsigned v){for(unsigned i=0;i<4;i++)s[o+i]=v>>(24-8*i);}
static unsigned word(const ArenaEntitySlot&s,unsigned o){unsigned v=0;for(unsigned i=0;i<4;i++)v=(v<<8)|s[o+i];return v;}
static void scalar(ArenaEntitySlot&s,unsigned o,float v){word(s,o,std::bit_cast<unsigned>(v));}
static float scalar(const ArenaEntitySlot&s,unsigned o){return std::bit_cast<float>(word(s,o));}
static ArenaEntitySlot tower(){ArenaEntitySlot s{};s[15]=22;s[0x33]=16;s[0x60]=1;s[0xba]=0;s[0xbb]=1;scalar(s,0xc8,-1);scalar(s,0xb4,1);s[0x90]=s[0x91]=255;return s;}
int main(){
 auto s=tower();auto r=advance_arena_tesla_source(s,.05f,0,123,false);assert(r&&r->spawn_beam&&r->target==123);assert(s[0x90]==0&&s[0x91]==123);assert(scalar(s,0xc8)<-1);assert(scalar(s,0xb4)==0);
 s=tower();r=advance_arena_tesla_source(s,.05f,0,{},true);assert(r&&!r->spawn_beam&&!r->target);assert(s[0x90]==255&&s[0x91]==255&&s[0xb9]==1);
 s=tower();r=advance_arena_tesla_source(s,.05f,1,123,true);assert(r&&!r->spawn_beam&&!r->target);assert(scalar(s,0xc8)>3.9f&&scalar(s,0xc8)<4);assert(s[0xb9]==0);
 scalar(s,0xc8,0);r=advance_arena_tesla_source(s,.05f,1,123,true);assert(r&&!r->target&&scalar(s,0xc8)<0);r=advance_arena_tesla_source(s,.05f,0,123,false);assert(r&&r->spawn_beam);
 s=tower();s[0x90]=0;s[0x91]=12;scalar(s,0xc8,.01f);r=advance_arena_tesla_source(s,.05f,0,123,true);assert(r&&!r->target&&s[0x91]==12&&scalar(s,0xc8)<0);r=advance_arena_tesla_source(s,.05f,0,123,false);assert(r&&r->spawn_beam&&s[0x91]==123&&scalar(s,0xc4)==.05f);
 s=tower();scalar(s,0xb4,std::bit_cast<float>(0x3eccccccu));r=advance_arena_tesla_source(s,0,0,123,true);assert(r&&scalar(s,0xb4)==std::bit_cast<float>(0x3eccccccu));
 s=tower();s[0x60]=0;auto old=s;r=advance_arena_tesla_source(s,.05f,0,123,false);assert(r&&!r->target&&s[0x90]==255);s=old;s[0xbb]=2;old=s;assert(!advance_arena_tesla_source(s,.05f,0,123,false)&&s==old);
 s=tower();old=s;assert(!advance_arena_tesla_source(s,std::numeric_limits<float>::infinity(),0,{},false)&&s==old);
 ArenaPackedCellEntry e{};e[3]=3;e[7]=(1<<5)|(5<<2)|2;auto score=arena_tesla_target_score(e,2.5f,0x10,1,2);assert(score&&*score==450339.5f);assert(!arena_tesla_target_score(e,2.5f,0,2,2));e[3]=0;assert(*arena_tesla_target_score(e,2.5f,0,1,2)==95.5f);e[3]=36;assert(*arena_tesla_target_score(e,2.5f,0,1,2)==62.5f);e[3]=3;e[7]=(1<<5)|(4<<2);assert(*arena_tesla_target_score(e,2.5f,0,1,2)==29.5f);e[7]=(1<<5)|(3<<2);assert(*arena_tesla_target_score(e,2.5f,0,1,2)==10);e[7]=(1<<5)|(6<<2);assert(!arena_tesla_target_score(e,2.5f,0,1,2));
 ArenaEntitySlot beam;beam.fill(0xa5);s=tower();scalar(s,0,4);scalar(s,4,.2f);scalar(s,8,6);s[0xc]=0x12;s[0xd]=0x34;s[0x6e]=2;assert(initialize_arena_tesla_beam(beam,s));assert(beam[15]==5&&beam[0x6f]==40&&beam[0x33]==0);assert(word(beam,0x3c)==0x42700000&&word(beam,0x38)==0x4e6e6b28&&word(beam,0x64)==0x4e6e6b28);assert(beam[0x1e]==0x12&&beam[0x1f]==0x34);assert(word(beam,0xe0)==0&&word(beam,0xe4)==0xffffffff&&word(beam,0xe8)==2);assert(word(beam,0xf0)==0&&word(beam,0xf8)==0&&beam[0xfc]==0&&beam[0xfd]==0xa5);for(unsigned o:{0x80,0x90,0xa0,0xb0,0xc0})assert(word(beam,o)==word(beam,0)&&word(beam,o+4)==word(beam,4));
 assert(!arena_tesla_cell_distance(0,0));assert(!arena_tesla_cell_distance(24,1));assert(*arena_tesla_cell_distance(3,4)==5.f);
 const int previous=fegetround();fesetround(FE_UPWARD);s=tower();assert(advance_arena_tesla_source(s,.05f,0,{},false));assert(fegetround()==FE_UPWARD);fesetround(previous);
}
