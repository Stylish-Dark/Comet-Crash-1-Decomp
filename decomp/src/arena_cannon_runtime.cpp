#include "arena_cannon_runtime.hpp"
#include "arena_weapon_target.hpp"
#include "comet/arena_cannon_source.hpp"
#include "comet/arena_projectile.hpp"
#include "comet/arena_ground_contact.hpp"
#include "comet/arena_ground_pair.hpp"
#include "comet/arena_impulse.hpp"
#include "comet/arena_construction_progress.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cfenv>
#include <stdexcept>
namespace comet::decomp {
namespace {
using V=std::array<float,4>;
struct Rounding {
 int previous=std::fegetround();
 Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO))throw std::runtime_error("cannon rounding unavailable");}
 ~Rounding(){std::fesetround(previous);}
};
unsigned half(const ArenaEntitySlot&s,unsigned o){return (unsigned(s[o])<<8)|s[o+1];}
void half(ArenaEntitySlot&s,unsigned o,unsigned v){s[o]=v>>8;s[o+1]=v;}
float scalar(const ArenaEntitySlot&s,unsigned o){unsigned w=0;for(unsigned i=0;i<4;i++)w=(w<<8)|s[o+i];return std::bit_cast<float>(w);}
V vector(const ArenaEntitySlot&s,unsigned o){return {scalar(s,o),scalar(s,o+4),scalar(s,o+8),0};}
bool ground(const ArenaEntitySlot&s){return (s[0x33]&28)==20&&(s[15]==40||s[15]==41||(s[15]>=43&&s[15]<=47));}
ArenaGridPosition cell(const ArenaEntityStorage&w,const ArenaEntitySlot&s){return {
 static_cast<std::uint8_t>(std::clamp(std::trunc(scalar(s,0)),0.f,float(w.width-1))),
 static_cast<std::uint8_t>(std::clamp(std::trunc(scalar(s,8)),0.f,float(w.height-1)))};}
}
ArenaCannonTick stage_arena_cannons(ArenaEntityStorage&w,const ArenaRoutingGrid&grid,
 std::span<const ArenaRoutePlayer> players,float dt,ArenaRandom&random,std::span<V> impulses){
 if(w.read_bank>1||!w.width||!w.height||w.width>24||w.height>24||
    w.banks[0].size()!=w.banks[1].size()||impulses.size()!=w.banks[0].size()||
    players.empty()||players.size()>4||!std::isfinite(dt)||dt<=0||dt>.05f)
  throw std::invalid_argument("invalid cannon world");
 std::array<std::uint8_t,4> teams{};for(unsigned p=0;p<players.size();p++)teams[p]=players[p].team;
 Rounding rounding;ArenaCannonTick tick;
 auto update=[&](unsigned i,const ArenaEntitySlot&s){
  if(update_arena_entity(w,static_cast<std::uint16_t>(i),s)!=ArenaEntityUpdate::Updated)
   throw std::invalid_argument("invalid cannon membership");
 };
 for(unsigned i=0;i<w.banks[0].size();i++){
  auto source=w.banks[w.read_bank][i];
  if(source[15]!=21||!arena_structure_ready(source))continue;
  const unsigned owner=source[0x33]>>5;if(owner>=players.size())throw std::invalid_argument("invalid cannon owner");
  ArenaCannonSourceQuery q;q.elapsed=dt;
  q.owner_base_center={players[owner].base.x+.5f,0,players[owner].base.z+.5f,0};
  const auto selected=select_arena_ground_weapon_target(w,grid,source,std::span(teams).first(players.size()));
  q.gathered_target=selected;
  const auto cached=half(source,0x1e);
  if(cached<w.banks[0].size()){
   const auto &target=w.banks[w.read_bank][cached];const unsigned target_owner=target[0x33]>>5;
   if(ground(target)&&target_owner<players.size()&&teams[target_owner]!=teams[owner]){
    ArenaCannonTarget t;t.index=static_cast<std::uint16_t>(cached);t.position=vector(target,0);
    t.velocity=vector(target,0x40);t.height_byte=target[0x1b];
    // Native admission uses the predicted range too; unsupported stale targets
    // are dropped and reacquired rather than aborting an otherwise valid tick.
    q.target=t;
   }
  }
  if(!q.target)half(source,0x1e,0xffff);
  auto result=advance_arena_cannon_source(source,q);
  if(!result&&q.target){ // Prediction outside range or unsupported intercept.
   half(source,0x1e,0xffff);q.target.reset();result=advance_arena_cannon_source(source,q);
  }
  if(!result)throw std::invalid_argument("unsupported cannon source state");
  if(result->fire&&!w.free_indices.empty()){
   ArenaEntitySlot projectile{};
   if(!initialize_arena_cannon_projectile(projectile,source,*result->fire))throw std::invalid_argument("invalid cannon projectile constructor");
   // Target belongs to this shot's cached snapshot, before source reacquisition.
   half(projectile,0x1c,cached);
   const auto inserted=insert_arena_entity(w,cell(w,projectile),projectile);
   if(inserted){impulses[*inserted]={};tick.spawned.push_back(*inserted);}
  }
  // Original fire gate consumes its timer before 2058 allocation, including failure.
  update(i,source);
 }
 for(unsigned i=0;i<w.banks[0].size();i++){
  auto projectile=w.banks[w.read_bank][i];if(projectile[15]!=3)continue;
  ArenaGroundBoundsQuery bounds;bounds.position=vector(projectile,0);bounds.velocity=vector(projectile,0x40);
  bounds.min_x=bounds.min_z=-2;bounds.max_x=float(w.width)+2;bounds.max_z=float(w.height)+2;
  bounds.min_y=std::bit_cast<float>(0xbe4cccccu);bounds.max_y=8;
  bounds.radius=float(projectile[14])*.01f;bounds.lateral_radius=float(projectile[0x32])*.01f;
  const unsigned flags=(unsigned(projectile[0x60])<<24)|(unsigned(projectile[0x61])<<16)|
                       (unsigned(projectile[0x62])<<8)|projectile[0x63];
  bounds.restitution_percent=projectile[0x7d];bounds.movable=(flags&0x4000)!=0;
  bounds.floor_uses_lateral_radius=(flags&0x02000000)!=0;
  const auto contained=contain_arena_ground_unit(bounds);
  if(!contained)throw std::invalid_argument("invalid projectile bounds");
  ArenaProjectileContact contact;contact.bounds_position=contained->position;contact.bounds_velocity=contained->velocity;
  const V gravity{0,-9.8f,0,0};V velocity=contained->velocity,predicted{};
  if(contained->position[1]>1)for(unsigned a=0;a<3;a++)velocity[a]=std::fma(gravity[a],dt,velocity[a]);
  for(unsigned a=0;a<3;a++)predicted[a]=std::fma(velocity[a],dt,contained->position[a]);
  const auto target_id=half(projectile,0x1c);
  if(target_id<w.banks[0].size()&&target_id!=half(projectile,0x6c)){
   const auto &target=w.banks[w.read_bank][target_id];
   if(target[15]>61||!ground(target))half(projectile,0x1c,0xffff);
   else {
    ArenaGroundPairQuery pair;pair.position=predicted;pair.peer_position=vector(target,0);
    pair.radius=bounds.radius;pair.peer_radius=target[14];pair.own_mass=scalar(projectile,0x58);pair.peer_mass=scalar(target,0x58);
    pair.index=static_cast<std::uint16_t>(i);pair.peer_index=static_cast<std::uint16_t>(target_id);
    pair.jitter=arena_ground_random_direction(random);
    const auto collision=prepare_arena_ground_pair(pair);if(!collision)throw std::invalid_argument("invalid cannon target geometry");
    contact.corrected_position=collision->position;
    contact.hit=collision->overlapping;contact.type=6;contact.target_index=static_cast<std::uint16_t>(target_id);
   }
  }
  // Native safety lifetime: missed shots cannot exhaust the finite entity pool.
  // This 600-update cap is preview policy, not an original lifetime claim.
  const bool expired=contained->energetic_impact ||
   std::any_of(predicted.begin(),predicted.begin()+3,[](float f){return !std::isfinite(f)||std::fabs(f)>64;}) ||
   scalar(projectile,0)<-2||scalar(projectile,8)<-2||
   scalar(projectile,0)>w.width+2||scalar(projectile,8)>w.height+2||
   ((unsigned(projectile[0x80])<<24)|(unsigned(projectile[0x81])<<16)|
    (unsigned(projectile[0x82])<<8)|projectile[0x83])>=600;
  const auto step=advance_arena_projectile(projectile,dt,contact,expired,gravity);
  if(step.status!=ArenaProjectileStatus::Active&&step.status!=ArenaProjectileStatus::Retire)
   throw std::invalid_argument("invalid cannon projectile state");
  if(step.impact_message){
   if(step.target_index>=impulses.size()||!accumulate_arena_impulse(impulses[step.target_index],step.impulse))throw std::invalid_argument("invalid cannon impulse");
   tick.impacts.push_back(step.target_index);
  }
  if(step.status==ArenaProjectileStatus::Retire){
   if(!retire_arena_entity(w,static_cast<std::uint16_t>(i)))throw std::invalid_argument("invalid projectile retirement");
   impulses[i]={};tick.retired.push_back(static_cast<std::uint16_t>(i));
  }else {
   const auto moved=update_arena_entity(w,static_cast<std::uint16_t>(i),projectile);
   if(moved==ArenaEntityUpdate::CellFull){
    // Native finite-cell pressure retires this shot; it must not abort a battle.
    if(!retire_arena_entity(w,static_cast<std::uint16_t>(i)))throw std::invalid_argument("invalid blocked projectile retirement");
    impulses[i]={};tick.retired.push_back(static_cast<std::uint16_t>(i));
   }else if(moved!=ArenaEntityUpdate::Updated)throw std::invalid_argument("invalid projectile membership");
  }
 }
 return tick;
}
}
