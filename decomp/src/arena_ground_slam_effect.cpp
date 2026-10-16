#include "comet/arena_ground_slam_effect.hpp"
#include "comet/arena_ground_slam.hpp"
#include "comet/arena_area_effect.hpp"
#include "comet/arena_health_delta.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
float scalar(const ArenaEntitySlot &s,unsigned off){
 unsigned word=0;for(unsigned i=0;i<4;++i)word=(word<<8)|s[off+i];return std::bit_cast<float>(word);
}
}
std::optional<std::vector<std::uint16_t>> accumulate_arena_ground_slam(
 const ArenaEntityStorage &world,std::uint16_t source,std::uint32_t owner_mask,
 const std::array<float,4> &impulse,std::span<float> pending){
 if(world.read_bank>1 || !world.width || !world.height || world.width>24 || world.height>24 ||
    source>=world.banks[0].size() || source>=world.banks[1].size() ||
    pending.size()!=world.banks[0].size() || world.banks[0].size()!=world.banks[1].size())return {};
 const auto &slot=world.banks[world.read_bank][source];
 if(slot[15]!=44 || (slot[0x33]&28)!=20 || ((unsigned(slot[12])<<8)|slot[13])!=source ||
    slot!=world.banks[1-world.read_bank][source])return {};
 const auto ready=arena_ground_slam_ready(impulse);if(!ready)return {};
 std::vector<std::uint16_t> hits;if(!*ready)return hits;
 const std::array<float,4> center{scalar(slot,0),scalar(slot,4),scalar(slot,8),0};
 for(float f:center)if(!std::isfinite(f) || std::abs(f)>64)return {};
 const int cx=std::clamp(int(std::trunc(center[0])),0,int(world.width)-1);
 const int cz=std::clamp(int(std::trunc(center[2])),0,int(world.height)-1);
 std::vector<float> staged(pending.begin(),pending.end());
 try {
  for(int x=std::max(0,cx-1);x<=std::min(int(world.width)-1,cx+1);++x)
   for(int z=std::max(0,cz-1);z<=std::min(int(world.height)-1,cz+1);++z){
    ArenaGridPosition cell{static_cast<std::uint8_t>(x),static_cast<std::uint8_t>(z)};
    constexpr float radius=std::bit_cast<float>(0x3f199999u);
    if(!arena_area_effect_cell_overlaps(center,cell,radius))continue;
    for(const auto &entry:world.cells[world.read_bank][z*24+x]){
     ArenaAreaEffectQuery q;q.center=center;q.cell=cell;q.entry=entry;q.stage_mask=32;
     q.owner_mask=owner_mask;q.radius=radius;q.damage=5;q.exclude_marker_two=true;
     const auto hit=evaluate_arena_area_effect_cubic(q);if(!hit)continue;
     const unsigned index=hit->index;
     if(index>=staged.size())return {};
     const auto &target=world.banks[world.read_bank][index];
     if(target!=world.banks[1-world.read_bank][index] ||
        ((unsigned(target[12])<<8)|target[13])!=index || (target[0x33]&28)!=20 ||
        !(target[15]==40 || target[15]==41 || (target[15]>=43 && target[15]<=47)))return {};
     if(!accumulate_arena_health_delta(staged[index],hit->health_delta))return {};
     hits.push_back(static_cast<std::uint16_t>(index));
    }
   }
 }catch(const std::invalid_argument&){return {};}
 std::copy(staged.begin(),staged.end(),pending.begin());return hits;
}
}
