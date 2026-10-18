#include "arena_weapon_target.hpp"
#include "comet/arena_tesla_source.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cfenv>
#include <stdexcept>
namespace comet::decomp {
namespace {
unsigned half(const ArenaEntitySlot&s,unsigned o){return (unsigned(s[o])<<8)|s[o+1];}
float scalar(const ArenaEntitySlot&s,unsigned o){unsigned w=0;for(unsigned i=0;i<4;i++)w=(w<<8)|s[o+i];return std::bit_cast<float>(w);}
bool ground(const ArenaEntitySlot&s){return (s[0x33]&28)==20 && (s[15]==40 || s[15]==41 || (s[15]>=43 && s[15]<=47));}
float add(float a,float b){volatile float v=a+b;return v;}
struct Rounding{int previous=std::fegetround();Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO))throw std::runtime_error("weapon rounding unavailable");}~Rounding(){std::fesetround(previous);}};
}
std::optional<std::uint16_t> select_arena_ground_weapon_target(const ArenaEntityStorage&w,const ArenaRoutingGrid&grid,
 const ArenaEntitySlot&source,std::span<const std::uint8_t> teams){
 Rounding rounding;
 const unsigned owner=source[0x33]>>5;
 if(owner>=teams.size())throw std::invalid_argument("invalid weapon owner");
 const float radius=scalar(source,0x5c);
 if(!std::isfinite(radius)||radius<0||radius>32)throw std::invalid_argument("invalid weapon range");
 const float px=scalar(source,0),pz=scalar(source,8);
 if(!std::isfinite(px)||!std::isfinite(pz)||px<0||pz<0||px>=w.width||pz>=w.height)throw std::invalid_argument("invalid weapon position");
 const int cx=int(px),cz=int(pz);float best=0;std::optional<std::uint16_t> selected;
 auto cell=[&](int x,int z,float distance){
  for(const auto &entry:w.cells[w.read_bank][z*24+x]){
   if(((entry[7]>>2)&7)!=5)continue; // Bounded native admission: ordinary ground units.
   const unsigned index=(unsigned(entry[0])<<8)|entry[1],peer_owner=entry[7]>>5;
   if(index>=w.banks[0].size()||peer_owner>=teams.size())throw std::invalid_argument("invalid weapon candidate");
   const auto &peer=w.banks[w.read_bank][index];
   if(!ground(peer))continue;
   if(half(peer,0xc)!=index||peer[0x33]!=entry[7]||peer!=w.banks[1-w.read_bank][index])throw std::invalid_argument("invalid weapon candidate membership");
   if(teams[peer_owner]==teams[owner])continue;
   const unsigned pair=3*peer_owner+owner-(owner>peer_owner?1:0);
   const auto score=arena_tesla_target_score(entry,distance,grid.cells[z*24+x].routes[pair],teams[owner],teams[peer_owner]);
   if(score && *score>best){best=*score;selected=static_cast<std::uint16_t>(index);}
  }
 };
 for(int z=std::max(0,cz-1);z<=std::min(int(w.height)-1,cz+1);++z)
  for(int x=std::max(0,cx-1);x<=std::min(int(w.width)-1,cx+1);++x)
   cell(x,z,(x!=cx&&z!=cz)?std::bit_cast<float>(0x3fb4fdf3u):1.f);
 if(selected)return selected;
 const int reach=int(add(radius,std::bit_cast<float>(0x3efae147u)));
 const float limit=add(radius,std::bit_cast<float>(0x3dccccccu));
 for(int z=std::max(0,cz-reach);z<=std::min(int(w.height)-1,cz+reach);++z)
  for(int x=std::max(0,cx-reach);x<=std::min(int(w.width)-1,cx+reach);++x){
   if(std::abs(x-cx)<=1 && std::abs(z-cz)<=1)continue;
   const auto distance=arena_tesla_cell_distance(x-cx,z-cz);
   if(!distance)throw std::invalid_argument("invalid weapon candidate distance");
   if(*distance<=limit)cell(x,z,*distance);
  }
 return selected;
}
}
