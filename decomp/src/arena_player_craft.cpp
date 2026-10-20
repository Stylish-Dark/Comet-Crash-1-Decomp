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
std::optional<std::array<float,4>> resolve_arena_player_craft_spawn(
 std::uint8_t owner,unsigned width,unsigned depth,float x,float z,float w) {
 if(owner>=4 || width<1 || width>24 || depth<1 || depth>24)return {};
 for(float f:{x,z,w})if(!std::isfinite(f) || std::abs(f)>1000000)return {};
 if(x<0) {
  // CFFC8..D001C: signed owner division/remainder (native owners are 0..3),
  // unsigned extent halves, -1, float conversion, then source f22=.5.
  x=float(int(width/2)+int(owner%2)-1)+.5f;
  z=float(int(depth/2)+int(owner/2)-1)+.5f;
 }
 return std::array<float,4>{x,0,z,w};
}
ArenaCraftRegistration register_arena_player_craft(ArenaEntityStorage& storage,
 ArenaPlayer& player,ArenaPlayerCraftRegistry& registry,std::uint8_t owner,
 const std::array<float,4>& spawn,const ArenaEntitySlot& reused_slot,
 const ArenaCraftAuxiliaryStart& auxiliary) {
 // Native API admission, outside the original successful bootstrap path.
 if(owner>=4 || storage.read_bank>1 || !storage.width || !storage.height ||
    storage.width>24 || storage.height>24 || spawn[1]!=0)
  return ArenaCraftRegistration::Invalid;
 for(float f:spawn)if(!std::isfinite(f) || std::abs(f)>1000000)
  return ArenaCraftRegistration::Invalid;
 if(spawn[0]<0 || spawn[0]>=storage.width || spawn[2]<0 || spawn[2]>=storage.height)
  return ArenaCraftRegistration::Invalid;
 if(player.craft_entity_index || registry.indices[owner])
  return ArenaCraftRegistration::AlreadyLinked;
 if(!auxiliary)return ArenaCraftRegistration::MissingAuxiliary;
 const ArenaGridPosition cell{static_cast<std::uint8_t>(spawn[0]),
                              static_cast<std::uint8_t>(spawn[2])};
 const auto flat=cell.z*24u+cell.x;
 if(storage.free_indices.empty() || storage.cells[0][flat].size()>=126 ||
    storage.cells[1][flat].size()>=126)return ArenaCraftRegistration::NoCapacity;
 const auto next=storage.free_indices.front();
 if(next>65535 || next>=storage.banks[0].size() || next>=storage.banks[1].size())
  return ArenaCraftRegistration::Invalid;
 // CFA54..CFA78: truncate source X/Z and issue category6/flag1/scalar0.
 // This return is deliberately ignored by the original caller.
 ArenaCraftAuxiliaryRequest request{6,cell.x,cell.z,1,0};
 auxiliary(request);
 // CFA80..CFB88 plus CA514 at CFB8C: shared scratch bytes are preserved,
 // then the body receives FIFO identity and ordered entries in both banks.
 auto slot=reused_slot;
 initialize_arena_player_craft_entity(slot,owner,spawn[0],spawn[2],spawn[3]);
 const auto index=insert_arena_entity(storage,cell,slot);
 if(!index)throw std::logic_error("craft registration changed during auxiliary call");
 // CFBC8 precedes CFBD8, and both precede the retained auxiliary calls.
 player.craft_entity_index=*index;
 registry.indices[owner]=*index;
 // CFBDC/CFBE8 and CFC00/CFC0C: retain even -1; do not invent audio success.
 player.craft_auxiliary_handles[0]=auxiliary(request);
 request.category=5;
 player.craft_auxiliary_handles[1]=auxiliary(request);
 return ArenaCraftRegistration::Registered;
}
}
