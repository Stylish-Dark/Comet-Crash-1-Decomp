#include "comet/arena_entity_spatial.hpp"
#include "arena_spu_math.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp { namespace {
using V=std::array<float,4>;
unsigned word(auto b,unsigned o){return unsigned(b[o])<<24|unsigned(b[o+1])<<16|unsigned(b[o+2])<<8|b[o+3];}
void put(auto b,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(24-8*i));}
float f(unsigned b){return std::bit_cast<float>(b);}
float mul(float a,float b){volatile float v=a*b;return v;}
float add(float a,float b){volatile float v=a+b;return v;}
float sub(float a,float b){volatile float v=a-b;return v;}
float ma(float a,float b,float c){return std::fma(a,b,c);}
float root(float s){float e=detail::inverse_root_estimate(s),v=mul(e,s);return ma(std::fma(-e,v,f(0x3f800001)),mul(v,.5f),v);}
bool number(float v){return std::isfinite(v)&&std::fpclassify(v)!=FP_SUBNORMAL&&std::abs(v)<=1e6f;}
struct Rounding{int old=std::fegetround();Rounding(){if(old<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("spatial rounding unavailable");}~Rounding(){std::fesetround(old);}};
}
ArenaSpatialNeighborhood arena_spatial_neighborhood(const ArenaEntityStorage& storage,ArenaGridPosition cell){
 if(storage.read_bank>1||!storage.width||!storage.height||storage.width>24||storage.height>24||cell.x>=storage.width||cell.z>=storage.height)throw std::invalid_argument("invalid original spatial job cell");
 ArenaSpatialNeighborhood result;result.cell_x=cell.x;result.cell_z=cell.z;
 for(int x=0;x<3;++x)for(int z=0;z<3;++z){const int cx=int(cell.x)+x-1,cz=int(cell.z)+z-1;
  if(cx>=0&&cz>=0&&cx<int(storage.width)&&cz<int(storage.height))result.cells[x+3*z]=storage.cells[storage.read_bank][24*cz+cx];
 }
 return result;
}
bool select_arena_entity_peer(std::span<std::uint8_t> work,const ArenaSpatialNeighborhood& neighborhood){
 if(work.size()<0x5c0||neighborhood.cell_x<0||neighborhood.cell_z<0||neighborhood.cell_x>23||neighborhood.cell_z>23)return false;
 V position;for(unsigned i=0;i<4;++i)position[i]=f(word(work,0x480+4*i));
 for(unsigned i=0;i<3;++i)if(!number(position[i]))return false;
 const float radius=f(word(work,0x4e0));if(!number(radius)||radius<0)return false;
 for(auto cell:neighborhood.cells)if(cell.size()>126)return false;
 if(!std::isfinite(position[3])||std::abs(position[3])>1e6f)return false;
 V best{100,100,100,100};unsigned selected=65535;
 Rounding rounding;
 for(int x=0;x<3;++x)for(int z=0;z<3;++z){
  const float cx=float(neighborhood.cell_x+x-1),cz=float(neighborhood.cell_z+z-1);
  const float dx=sub(position[0],add(cx,.5f)),dz=sub(position[2],add(cz,.5f));
  const float squared=ma(dz,dz,mul(dx,dx));
  if(squared==0)return false; // source estimate zero boundary remains unresolved
  if(!(sub(sub(root(squared),radius),f(0x3f353f7c))<0))continue;
  const auto entries=neighborhood.cells[x+3*z];
  for(unsigned n=0;n<entries.size();++n){
   const auto& entry=entries[n];const auto& following=(n&1)?entries[n-1]:n+1<entries.size()?entries[n+1]:neighborhood.tail[x+3*z];
   const unsigned category=(entry[7]>>2)&7;
   const std::array<unsigned,4> ids{unsigned(entry[0])<<8|entry[1],unsigned(entry[4])<<8|entry[5],unsigned(following[0])<<8|following[1],unsigned(following[4])<<8|following[5]};
   const std::array<unsigned,4> radii{entry[2],entry[6],following[2],following[6]};
   V delta{ sub(position[0],ma(float(entry[4]),f(0x3c23d70a),sub(cx,.5f))),
            sub(position[1],ma(float(entry[5]),f(0x3c23d70a),-.5f)),
            sub(position[2],ma(float(entry[6]),f(0x3c23d70a),sub(cz,.5f))),position[3]};
   V score;
   for(unsigned i=0;i<4;++i){const auto j=(i+1)%4,k=(i+2)%4;
    const float s=ma(delta[k],delta[k],ma(delta[j],delta[j],mul(delta[i],delta[i])));
    if(!std::isfinite(s)||s<=0||std::fpclassify(s)==FP_SUBNORMAL)return false;
    const float own_radius=f(word(work,0x4e0+4*i));if(!number(own_radius))return false;
    score[i]=ma(-float(radii[i]),f(0x3c23d70a),sub(root(s),own_radius));
   }
   for(unsigned i=0;i<4;++i){
    const auto self=word(work,0x4f0+4*i),mask=word(work,0x10+4*i);
    if(ids[i]!=self&&(mask&(1u<<(i==0?category:0)))&&score[i]<best[i]){best[i]=score[i];if(i==0)selected=ids[i];}
   }
  }
 }
 for(unsigned i=0;i<4;++i)put(work,0x5b0+4*i,std::bit_cast<unsigned>(best[i]));
 work[0x1c]=std::uint8_t(selected>>8);work[0x1d]=std::uint8_t(selected);return true;
}
}
