#include "comet/arena_laser_beam.hpp"
#include <cassert>
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
using namespace comet::decomp;
namespace {
void put(ArenaEntitySlot&s,unsigned o,unsigned v){for(unsigned i=0;i<4;i++)s[o+i]=v>>(24-8*i);}
unsigned word(const ArenaEntitySlot&s,unsigned o){unsigned v=0;for(unsigned i=0;i<4;i++)v=(v<<8)|s[o+i];return v;}
void fp(ArenaEntitySlot&s,unsigned o,float v){put(s,o,std::bit_cast<unsigned>(v));}
float scalar(const ArenaEntitySlot&s,unsigned o){return std::bit_cast<float>(word(s,o));}
struct Fixture {
 ArenaEntitySlot beam{},parent{},target{};
 ArenaRandom random{4};
 Fixture(){beam[15]=5;parent[15]=22;target[15]=40;put(parent,0x60,0x01000000);fp(parent,0xb0,75);fp(beam,0x3c,60);beam[0x1d]=3;beam[0x1f]=1;target[0x0d]=3;parent[0x91]=7;fp(target,0,1);fp(beam,0xe8,1);}
 ArenaLaserBeamStep step(float dt=.1f){return advance_arena_laser_beam(beam,parent,&target,dt,random);}
};
void draws(ArenaRandom actual,ArenaRandom expected,unsigned count){while(count--)expected.next();assert(actual.next()==expected.next());}
}
int main(){
 {Fixture f;auto before=f.beam;auto rng=f.random;auto r=f.step();assert(r.status==ArenaLaserBeamStatus::Active);assert(r.health_delta==-6.f&&r.extra_health_delta==0&&r.target_index==3);assert(r.parent_command_increment==0);assert(scalar(f.beam,0)==1);assert(scalar(f.beam,0x80)==0&&scalar(f.beam,0x84)==std::bit_cast<float>(0x3eccccccu));assert(scalar(f.beam,0xb0)==1&&scalar(f.beam,0xc0)==1);assert(f.beam[0x1d]==7&&word(f.beam,0xe4)==3);assert(scalar(f.beam,0x3c)==75);draws(f.random,rng,2);
  for(unsigned i=0;i<256;i++){bool changed=i<12||(i>=0x1c&&i<0x1e)||i==0x33||(i>=0x3c&&i<0x40)||(i>=0x60&&i<0x64)||(i>=0x80&&i<0xe0)||(i>=0xe4&&i<0xe8)||i==0xfc;if(!changed)assert(f.beam[i]==before[i]);}}
 {Fixture f;put(f.beam,0xe8,1);put(f.beam,0xd0,0x7fc00000);assert(f.step().status==ArenaLaserBeamStatus::Active);assert(word(f.beam,0xe8)==1);}
 {Fixture f;auto rng=f.random;assert(advance_arena_laser_beam(f.beam,f.parent,&f.target,.1f,f.random,true).status==ArenaLaserBeamStatus::Active);draws(f.random,rng,7);}
 {Fixture f;auto rng=f.random;assert(advance_arena_laser_beam(f.beam,f.parent,&f.target,std::bit_cast<float>(0x3a83126eu),f.random,true).status==ArenaLaserBeamStatus::Active);draws(f.random,rng,2);}
 // An absent target resets the idle endpoint, consuming two additional draws.
 {Fixture f;f.beam[0x1c]=f.beam[0x1d]=255;auto rng=f.random;auto r=f.step();assert(r.status==ArenaLaserBeamStatus::Active&&r.target_index==0xffff&&r.health_delta==0);assert(f.beam[0xfc]==1&&word(f.beam,0xe4)==0xffffffffu);assert(scalar(f.beam,0xec)>=.1f&&scalar(f.beam,0xec)<.9f);assert(scalar(f.beam,0xf8)==0);draws(f.random,rng,4);}
 {Fixture f;f.target[15]=63;f.beam[0xfc]=1;fp(f.beam,0xec,.5f);fp(f.beam,0,2);auto rng=f.random;f.step();assert(scalar(f.beam,0)==2&&word(f.beam,0xec)==0x3ecccccc);draws(f.random,rng,2);}
 {Fixture f;f.target[15]=63;f.beam[0xfc]=2;fp(f.beam,0xec,.5f);auto rng=f.random;f.step();assert(f.beam[0xfc]==1);draws(f.random,rng,4);}
 // Range is strict and prediction includes velocity and the stage-four lift.
 {Fixture f;fp(f.target,0,2.5f);assert(f.step().target_index==0xffff);}
 {Fixture f;fp(f.target,0,2.4f);fp(f.target,0x40,2);assert(f.step().target_index==0xffff);}
 {Fixture f;f.target[0x33]=16;f.target[14]=100;f.step();assert(word(f.beam,4)==0x3effffff);}
 {Fixture f;f.target[15]=46;fp(f.beam,0xf8,.05f);auto r=f.step();assert(r.parent_command_increment==0&&r.extra_health_delta==0);r=f.step();assert(r.parent_command_increment==1&&r.extra_health_delta==-1000);assert(r.health_delta==-7.5f);r=f.step();assert(r.parent_command_increment==1&&r.extra_health_delta==-1000);}
 // A missing, disabled, or cooling-down parent retires without using RNG.
 for(unsigned mode=0;mode<3;mode++){Fixture f;if(mode==0)f.parent[15]=63;if(mode==1)put(f.parent,0x60,0);if(mode==2)fp(f.parent,0xc8,.1f);auto expected=f.beam;expected[0x1e]=expected[0x1f]=255;auto rng=f.random;assert(f.step().status==ArenaLaserBeamStatus::Retire);assert(f.beam==expected);draws(f.random,rng,0);}
 // Invalid inputs preserve both complete state and RNG, including after math.
 for(unsigned mode=0;mode<8;mode++){Fixture f;float dt=.1f;if(mode==0)dt=0;if(mode==1)dt=std::numeric_limits<float>::denorm_min();if(mode==2)dt=std::numeric_limits<float>::infinity();if(mode==3)fp(f.target,0,65);if(mode==4)fp(f.target,0x40,1025);if(mode==5)fp(f.beam,0xec,std::numeric_limits<float>::quiet_NaN());if(mode==6)put(f.beam,0xe0,8);if(mode==7)fp(f.beam,0x3c,std::numeric_limits<float>::min());auto b=f.beam;auto rng=f.random;assert(f.step(dt).status==ArenaLaserBeamStatus::Invalid);assert(f.beam==b);draws(f.random,rng,0);}
 {Fixture f;f.beam[15]=2;auto b=f.beam;auto rng=f.random;assert(f.step().status==ArenaLaserBeamStatus::Unsupported);assert(f.beam==b);draws(f.random,rng,0);}
 {Fixture f;auto old=std::fegetround();std::fesetround(FE_UPWARD);f.step();assert(std::fegetround()==FE_UPWARD);std::fesetround(old);}
}
