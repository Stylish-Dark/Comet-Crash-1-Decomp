#include "comet/arena_player_craft.hpp"
#include "fixtures/player_craft.hpp"
#include <array>
#include <bit>
#include <cassert>
#include <cfenv>
#include <cmath>
#include <limits>
#include <stdexcept>
using namespace comet::decomp;
static unsigned word(const std::array<std::uint8_t,256>& b,unsigned o){return unsigned(b[o])<<24|unsigned(b[o+1])<<16|unsigned(b[o+2])<<8|b[o+3];}
int main(){
 for(const auto& fixture:player_craft_fixtures){
  std::array<std::uint8_t,256> bytes;auto seed=fixture.seed;
  for(auto& value:bytes){seed=1664525u*seed+1013904223u;value=std::uint8_t(seed>>24);}
  const auto& p=fixture.position;
  initialize_arena_player_craft_entity(bytes,std::uint8_t(fixture.owner),std::bit_cast<float>(p[0]),std::bit_cast<float>(p[2]),std::bit_cast<float>(p[3]));
  std::uint64_t hash=14695981039346656037ull;for(auto value:bytes){hash^=value;hash*=1099511628211ull;}assert(hash==fixture.output_hash);
 }

 std::array<std::uint8_t,256> b;b.fill(0xa5);
 initialize_arena_player_craft_entity(b,2,3.5f,4.5f,.125f);
 assert(word(b,0)==0x40600000 && word(b,4)==0 && word(b,8)==0x40900000);
 assert(b[12]==0xa5 && b[13]==0xa5 && b[14]==24 && b[15]==1);
 assert(b[0x32]==24 && b[0x33]==0x58 && b[0x7d]==100 && b[0x6f]==40 && word(b,0x60)==0xc006);
 assert(word(b,0x58)==0x428c0000 && word(b,0x38)==0x447a0000 && word(b,0x3c)==0x42c80000 && word(b,0x64)==0x447a0000);
 assert(word(b,0x10)==64 && word(b,0x78)==0x3f800000 && word(b,0x5c)==0x40000000);
 assert(word(b,0x94)==0x3f800000 && word(b,0xa4)==0x3f800000 && word(b,0xfc)==0x3f800000);
 assert(word(b,0xe0)==0x40600000 && word(b,0xe4)==0 && word(b,0xe8)==0x40900000 && word(b,0xec)==0x3e000000);
 for(auto o:{0x80,0x84,0x88,0x8c,0xb4,0xd0,0x70})assert(word(b,o)==0xa5a5a5a5);
 for(auto o:{0xb8,0xc4,0xc8,0xca,0xcc,0xce})assert(b[o]==255 && b[o+1]==255);
 assert(b[0xba]==1 && b[0xbb]==1 && b[0xbc]==1 && b[0xbd]==0 && b[0xbe]==0 && b[0xbf]==0 && b[0xc6]==2 && b[0xc7]==0xa5);
 const auto saved=b;
 for(unsigned invalid=0;invalid<4;++invalid){bool rejected=false;try{initialize_arena_player_craft_entity(b,invalid==0?4:0,invalid==1?std::numeric_limits<float>::infinity():1,invalid==2?1000001.f:1,invalid==3?std::numeric_limits<float>::quiet_NaN():0);}catch(const std::invalid_argument&){rejected=true;}assert(rejected && b==saved);}
 const auto previous=std::fegetround();std::fesetround(FE_UPWARD);
 auto bounds=arena_player_craft_bounds(24,13);
 assert(std::fegetround()==FE_UPWARD);std::fesetround(previous);
 assert(bounds.width==24 && bounds.depth==13 && std::bit_cast<unsigned>(bounds.radius)==0x3e75c28f && bounds.radius==bounds.lateral_radius);
 assert(bounds.restitution_percent==100 && bounds.movable && !bounds.floor_uses_lateral_radius);
 for(auto extents:{std::array<unsigned,2>{0,24},{24,25}}){bool rejected=false;try{arena_player_craft_bounds(extents[0],extents[1]);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);}
}
