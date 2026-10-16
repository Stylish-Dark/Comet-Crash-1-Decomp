#include "arena_tesla_runtime.hpp"
#include "comet/arena_tesla_source.hpp"
#include "comet/arena_laser_beam.hpp"
#include "comet/arena_health_delta.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace comet::decomp {
namespace {
unsigned half(const ArenaEntitySlot&s,unsigned o){return (unsigned(s[o])<<8)|s[o+1];}
float scalar(const ArenaEntitySlot&s,unsigned o){unsigned w=0;for(unsigned i=0;i<4;++i)w=(w<<8)|s[o+i];return std::bit_cast<float>(w);}
void put_float(ArenaEntitySlot&s,unsigned o,float f){const auto w=std::bit_cast<unsigned>(f);for(unsigned i=0;i<4;++i)s[o+i]=w>>(24-8*i);}
bool ground(const ArenaEntitySlot&s){return (s[0x33]&28)==20 && (s[15]==40 || s[15]==41 || (s[15]>=43 && s[15]<=47));}
struct Rounding{int previous=std::fegetround();Rounding(){if(previous<0 || std::fesetround(FE_TOWARDZERO))throw std::runtime_error("Tesla rounding unavailable");}~Rounding(){std::fesetround(previous);}};
float add(float a,float b){volatile float v=a+b;return v;}
std::optional<std::uint16_t> select_target(const ArenaEntityStorage&w,const ArenaRoutingGrid&grid,
 const ArenaEntitySlot&source,std::span<const std::uint8_t> teams){
 const unsigned owner=source[0x33]>>5;
 if(owner>=teams.size())throw std::invalid_argument("invalid Tesla owner");
 const float radius=scalar(source,0x5c);
 if(!std::isfinite(radius)||radius<0||radius>32)throw std::invalid_argument("invalid Tesla range");
 const float px=scalar(source,0),pz=scalar(source,8);
 if(!std::isfinite(px)||!std::isfinite(pz)||px<0||pz<0||px>=w.width||pz>=w.height)throw std::invalid_argument("invalid Tesla position");
 const int cx=int(px),cz=int(pz);float best=0;std::optional<std::uint16_t> selected;
 auto cell=[&](int x,int z,float distance){
  for(const auto &entry:w.cells[w.read_bank][z*24+x]){
   if(((entry[7]>>2)&7)!=5)continue; // Bounded native admission: ordinary ground units.
   const unsigned index=(unsigned(entry[0])<<8)|entry[1],peer_owner=entry[7]>>5;
   if(index>=w.banks[0].size()||peer_owner>=teams.size())throw std::invalid_argument("invalid Tesla candidate");
   const auto &peer=w.banks[w.read_bank][index];
   if(!ground(peer))continue;
   if(half(peer,0xc)!=index||peer[0x33]!=entry[7]||peer!=w.banks[1-w.read_bank][index])throw std::invalid_argument("invalid Tesla candidate membership");
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
   if(!distance)throw std::invalid_argument("invalid Tesla candidate distance");
   if(*distance<=limit)cell(x,z,*distance);
  }
 return selected;
}
}
ArenaTeslaTick stage_arena_teslas(ArenaEntityStorage&w,const ArenaRoutingGrid&grid,
 std::span<const std::uint8_t> teams,float elapsed,ArenaRandom&random,
 std::span<float> health,std::span<std::uint32_t> commands){
 if(w.read_bank>1 || !w.width || !w.height || w.width>24 || w.height>24 ||
    w.banks[0].size()!=w.banks[1].size() || health.size()!=w.banks[0].size() || commands.size()!=health.size() ||
    teams.empty() || teams.size()>4)throw std::invalid_argument("invalid Tesla world");
 Rounding rounding;ArenaTeslaTick tick;
 auto update=[&](unsigned index,const ArenaEntitySlot &slot){
  if(update_arena_entity(w,static_cast<std::uint16_t>(index),slot)!=ArenaEntityUpdate::Updated)
   throw std::invalid_argument("invalid Tesla update membership");
 };
 // Native source-before-beam ordering; asynchronous original job snapshots are separate.
 for(unsigned i=0;i<w.banks[0].size();++i){
  if(w.banks[w.read_bank][i][15]!=22)continue;
  auto source=w.banks[w.read_bank][i];const auto before=source;bool exists=false;
  for(const auto &beam:w.banks[w.read_bank])if(beam[15]==5 && half(beam,0x1e)==i){
   if(exists)throw std::invalid_argument("unsupported multi-beam Tesla");
   exists=true;
  }
  const auto selected=select_target(w,grid,source,teams);
  const auto result=advance_arena_tesla_source(source,elapsed,commands[i],selected,exists);
  if(!result)throw std::invalid_argument("unsupported Tesla source state");
  commands[i]=0;
  bool inserted_beam=false;
  if(result->spawn_beam && !w.free_indices.empty()){
   const unsigned index=w.free_indices.front();
   if(index>=w.banks[0].size())throw std::invalid_argument("invalid Tesla allocation index");
   auto beam=w.banks[w.read_bank][index];
   if(!initialize_arena_tesla_beam(beam,source))throw std::invalid_argument("invalid Tesla beam constructor");
   const ArenaGridPosition cell{static_cast<std::uint8_t>(scalar(beam,0)),static_cast<std::uint8_t>(scalar(beam,8))};
   const auto inserted=insert_arena_entity(w,cell,beam);
   if(inserted){inserted_beam=true;source[0xb8]=0;source[0xb9]=1;health[*inserted]=0;commands[*inserted]=0;tick.spawned.push_back(*inserted);}
  }
  if(result->spawn_beam && !inserted_beam){
   // Original target assignment/acquisition sound reset follow successful
   // allocation. The ready timer still advances when the pool/cell is full.
   source[0x90]=before[0x90];source[0x91]=before[0x91];
   put_float(source,0xb4,add(scalar(before,0xb4),elapsed));
  }
  update(i,source);
 }
 for(unsigned i=0;i<w.banks[0].size();++i){
  if(w.banks[w.read_bank][i][15]!=5)continue;
  auto beam=w.banks[w.read_bank][i];const unsigned parent_id=half(beam,0x1e),target_id=half(beam,0x1c);
  ArenaEntitySlot missing{};
  const auto &parent=parent_id<w.banks[0].size()?w.banks[w.read_bank][parent_id]:missing;
  const ArenaEntitySlot *target=target_id<w.banks[0].size() && ground(w.banks[w.read_bank][target_id])?&w.banks[w.read_bank][target_id]:nullptr;
  const auto step=advance_arena_laser_beam(beam,parent,target,elapsed,random);
  if(step.status==ArenaLaserBeamStatus::Retire){
   if(!retire_arena_entity(w,static_cast<std::uint16_t>(i)))throw std::invalid_argument("invalid beam retirement");
   health[i]=0;commands[i]=0;tick.retired.push_back(static_cast<std::uint16_t>(i));continue;
  }
  if(step.status!=ArenaLaserBeamStatus::Active)throw std::invalid_argument("invalid Tesla beam state");
  if(step.target_index!=0xffff){
   if(step.target_index>=health.size() || !ground(w.banks[w.read_bank][step.target_index]) ||
      !accumulate_arena_health_delta(health[step.target_index],step.health_delta) ||
      !accumulate_arena_health_delta(health[step.target_index],step.extra_health_delta))throw std::invalid_argument("invalid Tesla hit");
  }
  if(step.parent_command_increment){
   if(parent_id>=commands.size() || commands[parent_id]>std::numeric_limits<std::uint32_t>::max()-step.parent_command_increment)
    throw std::invalid_argument("Tesla command overflow");
   commands[parent_id]+=step.parent_command_increment;
  }
  update(i,beam);
 }
 return tick;
}
}
