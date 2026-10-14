#include "comet/arena_ground_transport.hpp"
#include "comet/arena_ground_dynamics.hpp"
#include "comet/arena_ground_contact.hpp"
#include "comet/arena_ground_pair.hpp"
#include "comet/arena_ground_terrain.hpp"
#include <algorithm>
#include <bitset>
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
#include <limits>
namespace comet::decomp {
namespace {
std::uint32_t word(const ArenaEntitySlot &s, unsigned o) {
  std::uint32_t v=0; for(unsigned i=0;i<4;++i)v=(v<<8)|s[o+i]; return v;
}
float scalar(const ArenaEntitySlot &s,unsigned o) { return std::bit_cast<float>(word(s,o)); }
void put(ArenaEntitySlot &s,unsigned o,std::uint32_t v) {
  for(unsigned i=0;i<4;++i)s[o+i]=static_cast<std::uint8_t>(v>>(24-8*i));
}
struct Rounding {
  int previous=std::fegetround();
  Rounding() {
    if(previous<0 || std::fesetround(FE_TOWARDZERO)!=0)
      throw std::runtime_error("transport rounding unavailable");
  }
  ~Rounding() { std::fesetround(previous); }
};
ArenaGridPosition cell_for(const ArenaEntitySlot &s,unsigned width,unsigned height) {
  return {static_cast<std::uint8_t>(std::clamp(std::trunc(scalar(s,0)),0.f,float(width-1))),
          static_cast<std::uint8_t>(std::clamp(std::trunc(scalar(s,8)),0.f,float(height-1)))};
}
bool matches(const ArenaPackedCellEntry &e,std::uint16_t index) {
  return ((unsigned(e[0])<<8)|e[1])==index;
}
}
ArenaGroundTransport transport_arena_ground_unit(
    ArenaEntityStorage &storage, std::uint16_t index, float elapsed,
    std::span<const ArenaUnitTargetPlayer> players, const ArenaRoutingGrid &grid,
    ArenaRandom &random, std::span<const ArenaGridPosition> bases,
    std::span<const std::uint8_t> visibility_bits, bool original_velocity_dynamics, bool motion_contacts) {
  if(!std::isfinite(elapsed)||elapsed<=0||elapsed>.25f || storage.read_bank>1 ||
     storage.width==0||storage.height==0||storage.width>24||storage.height>24 ||
     index>=storage.banks[0].size()||index>=storage.banks[1].size() ||
     players.empty()||players.size()>4) return ArenaGroundTransport::Invalid;
  if((!bases.empty() && bases.size()!=players.size()) ||
     (!visibility_bits.empty() && (visibility_bits.size()!=players.size() || bases.empty())))
    return ArenaGroundTransport::Invalid;
  auto slot=storage.banks[storage.read_bank][index];
  const unsigned kind=slot[15],owner=slot[0x33]>>5;
  if(owner>=players.size() || !(kind==40||kind==41||(kind>=43&&kind<=47)) ||
     (slot[0x33]&28)!=20 || ((unsigned(slot[12])<<8)|slot[13])!=index)
    return ArenaGroundTransport::Invalid;
  for(unsigned o:{0u,4u,8u,0x84u})
    if(!std::isfinite(scalar(slot,o)))return ArenaGroundTransport::Invalid;
  if(storage.banks[0][index]!=storage.banks[1][index])return ArenaGroundTransport::Invalid;
  const float speed=scalar(slot,0x84);
  if(!std::isnormal(speed)||speed<=0)return ArenaGroundTransport::Invalid;
  const auto old_cell=cell_for(slot,storage.width,storage.height);
  const unsigned old_flat=old_cell.z*24+old_cell.x;
  std::array<std::size_t,2> offsets{};
  for(unsigned bank=0;bank<2;++bank) {
    const auto &list=storage.cells[bank][old_flat];
    auto it=std::find_if(list.begin(),list.end(),[&](const auto&e){return matches(e,index);});
    if(it==list.end()||std::count_if(list.begin(),list.end(),[&](const auto&e){return matches(e,index);})!=1)
      return ArenaGroundTransport::Invalid;
    offsets[bank]=static_cast<std::size_t>(it-list.begin());
  }
  if(!storage.summary_counts[old_flat])return ArenaGroundTransport::Invalid;
  auto next_random=random;
  std::array<ArenaUnitTargetPlayer,4> target_players{};
  std::copy(players.begin(),players.end(),target_players.begin());
  // Retain an existing live enemy target. The original binding helper's forced
  // path samples its directed route without consuming another selection draw.
  const unsigned target=slot[0x7e];
  if(players[owner].override_target==255 && target<players.size() && target!=owner &&
     players[target].base_available && players[target].team!=players[owner].team)
    target_players[owner].override_target=static_cast<std::uint8_t>(target);
  slot[0xc8]=slot[0xca]=0;slot[0xc9]=old_cell.x;slot[0xcb]=old_cell.z;
  const auto binding=bind_arena_ground_unit_template(slot,
    std::span(target_players.data(),players.size()),grid,next_random);
  if(binding==ArenaGroundUnitBinding::NoTarget)return ArenaGroundTransport::NoTarget;
  if(binding!=ArenaGroundUnitBinding::Bound && binding!=ArenaGroundUnitBinding::Blocked)
    return ArenaGroundTransport::Invalid;
  Rounding rounding;
  if(motion_contacts && !original_velocity_dynamics)return ArenaGroundTransport::Invalid;
  if(original_velocity_dynamics) {
    if(bases.empty())return ArenaGroundTransport::Invalid;
    // Binding selects target but must not replace accumulated live velocity.
    for(unsigned axis=0;axis<4;++axis) {
      const float previous=scalar(storage.banks[storage.read_bank][index],0x40+4*axis);
      if(!std::isfinite(previous))return ArenaGroundTransport::Invalid;
      put(slot,0x40+4*axis,std::bit_cast<std::uint32_t>(previous));
    }
    if(motion_contacts) {
      // Native broad phase: ordered nearby bank records, friendly ground units.
      // Original gather order and enemy capture remain separate. Pair geometry,
      // normal jitter and motion response use recovered arithmetic.
      const int reach=static_cast<int>(std::ceil(float(slot[14])*.01f+2.55f));
      std::bitset<65536> visited;
      for(int z=std::max(0,int(old_cell.z)-reach);z<=std::min(int(storage.height)-1,int(old_cell.z)+reach);++z)
        for(int x=std::max(0,int(old_cell.x)-reach);x<=std::min(int(storage.width)-1,int(old_cell.x)+reach);++x)
          for(const auto &entry:storage.cells[storage.read_bank][z*24+x]) {
            const unsigned other=(unsigned(entry[0])<<8)|entry[1];
            if(other==index || visited.test(other))continue;
            visited.set(other);
            if(other>=storage.banks[0].size()||other>=storage.banks[1].size())return ArenaGroundTransport::Invalid;
            const auto &peer=storage.banks[storage.read_bank][other];
            const unsigned peer_kind=peer[15],peer_owner=peer[0x33]>>5;
            if(!(peer_kind==40||peer_kind==41||(peer_kind>=43&&peer_kind<=47))||(peer[0x33]&28)!=20)continue;
            if(peer_owner>=players.size())return ArenaGroundTransport::Invalid;
            if(players[peer_owner].team!=players[owner].team)continue;
            if(peer!=storage.banks[1-storage.read_bank][other] || ((unsigned(peer[12])<<8)|peer[13])!=other)return ArenaGroundTransport::Invalid;
            ArenaGroundPairQuery pair;
            ArenaGroundContactQuery c;
            for(unsigned axis=0;axis<3;++axis) {
              pair.position[axis]=scalar(slot,4*axis);pair.peer_position[axis]=scalar(peer,4*axis);
              c.velocity[axis]=scalar(slot,0x40+4*axis);c.peer_velocity[axis]=scalar(peer,0x40+4*axis);
            }
            volatile float own_radius=float(slot[14])*std::bit_cast<float>(0x3c23d70au);
            pair.radius=own_radius;pair.peer_radius=peer[14];pair.own_mass=scalar(slot,0x58);pair.peer_mass=scalar(peer,0x58);
            pair.index=index;pair.peer_index=static_cast<std::uint16_t>(other);
            pair.jitter=arena_ground_random_direction(next_random);
            const auto prepared=prepare_arena_ground_pair(pair);if(!prepared)return ArenaGroundTransport::Invalid;
            // Original coincidence correction writes position even without overlap.
            for(unsigned axis=0;axis<3;++axis)put(slot,4*axis,std::bit_cast<std::uint32_t>(prepared->position[axis]));
            if(!prepared->overlapping)continue;
            c.position=prepared->position;c.normal=prepared->normal;c.response_share=prepared->response_share;
            c.penetration=-prepared->gap;c.restitution_percent=slot[0x7d];c.movable=(word(slot,0x60)&0x4000)!=0;
            const auto response=resolve_arena_ground_contact(c);if(!response)return ArenaGroundTransport::Invalid;
            for(unsigned axis=0;axis<3;++axis) {
              put(slot,4*axis,std::bit_cast<std::uint32_t>(response->position[axis]));
              put(slot,0x40+4*axis,std::bit_cast<std::uint32_t>(response->velocity[axis]));
            }
          }
    }
    // Original 14060 prepares position BEFORE 158B8 updates velocity.
    for(unsigned axis=0;axis<3;++axis) {
      if(axis==1 && !motion_contacts)continue;
      const float value=std::fma(scalar(slot,0x40+4*axis),elapsed,scalar(slot,4*axis));
      if(!std::isfinite(value))return ArenaGroundTransport::Invalid;
      const float limit=std::nextafter(float(axis==0?storage.width:storage.height),0.f);
      put(slot,4*axis,std::bit_cast<std::uint32_t>(motion_contacts?value:std::clamp(value,0.f,limit)));
    }
    if(motion_contacts) {
      ArenaGroundTerrainQuery terrain;
      auto &bounds=terrain.bounds;
      terrain.geometry.cell=old_cell;terrain.geometry.width=static_cast<std::uint8_t>(storage.width);
      terrain.geometry.height=static_cast<std::uint8_t>(storage.height);
      terrain.geometry.visibility_bit=visibility_bits.empty()?players[owner].team:visibility_bits[owner];
      if(terrain.geometry.visibility_bit>=4)return ArenaGroundTransport::Invalid;
      // 9D58..9EC0: the bypass flag is current-cell obstruction unless entity
      // bit 0x80000 suppresses it. 490 retains original X/Z with Y cleared.
      terrain.geometry.world_only=(word(slot,0x60)&0x80000)==0 &&
          (grid.cells[old_flat].visibility&(1u<<terrain.geometry.visibility_bit))!=0;
      terrain.reference_position={scalar(storage.banks[storage.read_bank][index],0),0,
                                  scalar(storage.banks[storage.read_bank][index],8),0};
      for(unsigned axis=0;axis<3;++axis){bounds.position[axis]=scalar(slot,4*axis);bounds.velocity[axis]=scalar(slot,0x40+4*axis);}
      bounds.max_x=float(storage.width);bounds.max_z=float(storage.height);
      // 7DD0 supplies floor=0, ceiling=2; 9A70 derives the two radii.
      volatile float radius=float(slot[14])*std::bit_cast<float>(0x3c23d70au);
      volatile float lateral=float(slot[0x32])*std::bit_cast<float>(0x3c23d70au);
      bounds.radius=radius;bounds.lateral_radius=lateral;bounds.restitution_percent=slot[0x7d];
      bounds.movable=(word(slot,0x60)&0x4000)!=0;bounds.floor_uses_lateral_radius=(word(slot,0x60)&0x02000000)!=0;
      const auto result=contain_arena_ground_terrain(terrain,grid);if(!result)return ArenaGroundTransport::Invalid;
      for(unsigned axis=0;axis<3;++axis){
        put(slot,4*axis,std::bit_cast<std::uint32_t>(result->position[axis]));
        put(slot,0x40+4*axis,std::bit_cast<std::uint32_t>(result->velocity[axis]));
      }
    }
  }
  if(!bases.empty()) {
    ArenaGroundSteeringQuery query;
    for(unsigned axis=0;axis<4;++axis) {
      query.position[axis]=scalar(slot,4*axis);
      query.orientation[axis]=scalar(storage.banks[storage.read_bank][index],0x20+4*axis);
    }
    query.owner=static_cast<std::uint8_t>(owner);query.target=slot[0x7e];
    query.extent=static_cast<std::uint8_t>(storage.width);
    if(storage.width!=storage.height)return ArenaGroundTransport::Invalid;
    query.target_available=players[query.target].base_available;
    query.target_base=bases[query.target];
    query.visibility_bit=visibility_bits.empty()?players[owner].team:visibility_bits[owner];
    const auto steering=steer_arena_ground_unit(query,grid);
    if(!steering)return ArenaGroundTransport::Invalid;
    if(steering->blocked)return ArenaGroundTransport::Blocked;
    if(original_velocity_dynamics) {
      ArenaGroundDynamicsQuery dynamics;
      for(unsigned axis=0;axis<4;++axis)dynamics.velocity[axis]=scalar(slot,0x40+4*axis);
      dynamics.steering=steering->direction;
      dynamics.steering_acceleration=scalar(slot,0x80);
      dynamics.speed_limit=speed;dynamics.elapsed=elapsed;
      dynamics.kind=static_cast<std::uint8_t>(kind);
      // 9C98/9D44 derive this context mask from the entity's stage bits.
      dynamics.context_flags=1u<<((slot[0x33]>>2)&7);
      // World/crowd acceleration input is intentionally zero in this preview.
      const auto result=advance_arena_ground_velocity(dynamics);
      if(!result)return ArenaGroundTransport::Invalid;
      for(unsigned axis=0;axis<4;++axis)
        put(slot,0x40+4*axis,std::bit_cast<std::uint32_t>(result->velocity[axis]));
    } else {
      for(unsigned axis=0;axis<4;++axis) {
        volatile float velocity=(axis==0 || axis==2)?steering->direction[axis]*speed:0.f;
        put(slot,0x40+4*axis,std::bit_cast<std::uint32_t>(float(velocity)));
      }
    }
    auto heading=steering->route_direction?arena_ground_route_heading(*steering->route_direction):
        std::optional<std::array<float,4>>(query.orientation);
    for(unsigned axis=0;axis<4;++axis)
      put(slot,0x20+4*axis,std::bit_cast<std::uint32_t>((*heading)[axis]));
  } else if(binding==ArenaGroundUnitBinding::Blocked)return ArenaGroundTransport::Blocked;
  // The original 141BC..141EC position update uses fused velocity*delta+position.
  // Keeping altitude fixed and applying that update only to x/z is the explicit
  // native transport policy; collision and auxiliary state remain unrecovered.
  if(!original_velocity_dynamics) for(unsigned axis:{0u,2u}) {
    const float value=std::fma(scalar(slot,0x40+4*axis),elapsed,scalar(slot,4*axis));
    if(!std::isfinite(value))return ArenaGroundTransport::Invalid;
    const float limit=std::nextafter(float(axis==0?storage.width:storage.height),0.f);
    put(slot,4*axis,std::bit_cast<std::uint32_t>(std::clamp(value,0.f,limit)));
  }
  const auto cell=cell_for(slot,storage.width,storage.height);
  const unsigned flat=cell.z*24+cell.x;
  if(flat!=old_flat) {
    const unsigned chosen=slot[0x7e];
    const unsigned pair=3*owner+chosen-(chosen>owner?1:0);
    // Walk every crossed grid boundary. Endpoint-only checks could jump over a
    // blocked intermediate cell at a high speed or long caller delta.
    int x=old_cell.x,z=old_cell.z;
    const double start_x=scalar(storage.banks[storage.read_bank][index],0);
    const double start_z=scalar(storage.banks[storage.read_bank][index],8);
    const double dx=scalar(slot,0)-start_x,dz=scalar(slot,8)-start_z;
    const int sx=dx>0?1:-1,sz=dz>0?1:-1;
    auto blocked=[&](int bx,int bz) {
      return bx<0||bz<0||bx>=int(storage.width)||bz>=int(storage.height)||
             (grid.cells[bz*24+bx].routes[pair]&1);
    };
    while(x!=cell.x||z!=cell.z) {
      const double tx=x==cell.x?std::numeric_limits<double>::infinity():
          ((sx>0?x+1:x)-start_x)/dx;
      const double tz=z==cell.z?std::numeric_limits<double>::infinity():
          ((sz>0?z+1:z)-start_z)/dz;
      if(tx==tz) {
        if(blocked(x+sx,z)||blocked(x,z+sz))return ArenaGroundTransport::Blocked;
        x+=sx;z+=sz;
      } else if(tx<tz)x+=sx;
      else z+=sz;
      if(blocked(x,z))return ArenaGroundTransport::Blocked;
    }
    if(storage.cells[0][flat].size()>=126||storage.cells[1][flat].size()>=126)
      return ArenaGroundTransport::CellFull;
  }
  slot[0xc8]=slot[0xca]=0;slot[0xc9]=cell.x;slot[0xcb]=cell.z;
  ArenaPackedCellEntry packed{slot[12],slot[13],slot[14],slot[0x6f],0,0,0,slot[0x33]};
  const std::array<float,3> origin{float(cell.x)-.5f,-.5f,float(cell.z)-.5f};
  for(unsigned axis=0;axis<3;++axis) {
    volatile float local=scalar(slot,4*axis)-origin[axis];
    volatile float scaled=std::clamp(float(local),0.f,2.f)*100.f;
    packed[4+axis]=static_cast<std::uint8_t>(scaled);
  }
  // All allocations precede visible writes. A blocked/full destination leaves
  // both banks, lists, counters, pool and RNG unchanged.
  if(flat!=old_flat)
    for(auto &bank:storage.cells)bank[flat].reserve(bank[flat].size()+1);
  for(unsigned bank=0;bank<2;++bank) {
    auto &list=storage.cells[bank][old_flat];
    if(flat==old_flat)list[offsets[bank]]=packed;
    else {list.erase(list.begin()+offsets[bank]);storage.cells[bank][flat].push_back(packed);}
    storage.banks[bank][index]=slot;
  }
  if(flat!=old_flat){--storage.summary_counts[old_flat];++storage.summary_counts[flat];}
  random=next_random;
  return ArenaGroundTransport::Moved;
}
} // namespace comet::decomp
