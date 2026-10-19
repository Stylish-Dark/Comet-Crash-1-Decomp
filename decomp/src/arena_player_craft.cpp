#include "comet/arena_player_craft.hpp"
#include "comet/arena_entity_core.hpp"
#include <array>
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
void initialize_arena_player_craft_entity(std::span<std::uint8_t,256> bytes,
 std::uint8_t owner,float x,float z,float staged_w){
 if(owner>=4)throw std::invalid_argument("invalid craft owner");
 for(float f:{x,z,staged_w})if(!std::isfinite(f)||std::abs(f)>1000000)
  throw std::invalid_argument("invalid craft spawn vector");
 ArenaEntityCoreParameters p;p.type=1;p.model_slot=40;p.owner=owner;p.stage=6;
 p.quantity=24;p.flags=0xc000;p.position={x,0,z,staged_w};
 p.scalar_58=70;p.scalar_3c=100;p.scalar_38=p.scalar_64=1000;
 initialize_arena_entity_core(bytes.first<128>(),p);
 auto word=[&](unsigned o,std::uint32_t value){for(unsigned i=0;i<4;++i)bytes[o+i]=std::uint8_t(value>>(24-8*i));};
 auto scalar=[&](unsigned o,float value){word(o,std::bit_cast<std::uint32_t>(value));};
 for(unsigned o:{0x90,0xa0,0xf0})for(unsigned i=0;i<4;++i)scalar(o+4*i,i==(o==0xf0?3u:1u)?1.f:0.f);
 for(unsigned i=0;i<4;++i)scalar(0xe0+4*i,p.position[i]);
 word(0x10,64);bytes[0x7d]=100;scalar(0x78,1);
 for(unsigned o:{0x74,0xb0,0xc0,0xd8,0xdc})word(o,0);
 scalar(0xd4,2);scalar(0x5c,2);
 bytes[0xba]=bytes[0xbb]=bytes[0xbc]=1;
 bytes[0xbd]=bytes[0xbe]=bytes[0xbf]=0;bytes[0xc6]=owner;
 for(unsigned o:{0xb8,0xc4,0xc8,0xca,0xcc,0xce})bytes[o]=bytes[o+1]=255;
}
ArenaCraftBounds arena_player_craft_bounds(unsigned width,unsigned depth){
 if(width<1||width>24||depth<1||depth>24)throw std::invalid_argument("invalid craft arena extent");
 std::array<std::uint8_t,256> slot{};initialize_arena_player_craft_entity(slot,0,0,0);
 const auto previous=std::fegetround();
 if(previous<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("craft bounds rounding unavailable");
 // SPU 9B90/9C60 convert the two radius bytes with this source multiplier.
 const float percent=std::bit_cast<float>(0x3c23d70au);
 volatile float radius=float(slot[14])*percent,lateral=float(slot[0x32])*percent;
 std::fesetround(previous);
 ArenaCraftBounds result;result.width=width;result.depth=depth;
 result.radius=radius;result.lateral_radius=lateral;result.restitution_percent=slot[0x7d];
 const auto flags=std::uint32_t(slot[0x60])<<24|std::uint32_t(slot[0x61])<<16|std::uint32_t(slot[0x62])<<8|slot[0x63];
 result.movable=(flags&0x4000)!=0;result.floor_uses_lateral_radius=(flags&0x02000000)!=0;
 return result;
}
}
