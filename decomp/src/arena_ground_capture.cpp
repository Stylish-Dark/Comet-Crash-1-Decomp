#include "comet/arena_ground_capture.hpp"
#include "comet/arena_player_statistics.hpp"
#include <algorithm>
#include <stdexcept>
namespace comet::decomp {
namespace {
unsigned word(const ArenaEntitySlot&s,unsigned o){unsigned v=0;for(unsigned i=0;i<4;++i)v=(v<<8)|s[o+i];return v;}
bool ground(unsigned kind){return kind==40||kind==41||(kind>=43&&kind<=47);}
}
ArenaGroundCapture capture_arena_ground_unit(ArenaEntitySlot &victim,const ArenaEntitySlot &peer,
 std::span<const std::uint8_t> teams,std::span<ArenaPlayerStatisticsBlock> statistics,bool overlapping){
 const unsigned owner=victim[0x33]>>5,recipient=peer[0x33]>>5;
 if(teams.empty()||teams.size()>4||statistics.size()!=teams.size()||owner>=teams.size()||recipient>=teams.size())return ArenaGroundCapture::Invalid;
 if(!overlapping||teams[owner]==teams[recipient]||peer[15]!=45||peer[0xbb]==0||
    (word(peer,0x60)&0x20000)!=0||(victim[0x33]&28)!=20)return ArenaGroundCapture::NotEligible;
 if(!ground(victim[15])||(peer[0x33]&28)!=20)return ArenaGroundCapture::Unsupported;
 // 83D8..83F0: kind-45 victims lose health instead of changing ownership.
 if(victim[15]==45){
  std::fill_n(victim.begin()+0x38,4,0);return ArenaGroundCapture::HealthDepleted;
 }
 auto destination=statistics[recipient],source=statistics[owner];
 // 83F4..8454: admit to the new owner before removing from the old owner.
 // Capacity rejection preserves the original admission's partial statistics
 // updates, but does not change ownership or remove source accounting.
 ArenaEntityRegistration admitted;
 try{
  admitted=register_arena_entity_statistics(destination,victim,true,true);
  if(admitted==ArenaEntityRegistration::Registered)
   register_arena_entity_statistics(source,victim,false,true);
 }catch(const std::invalid_argument&){return ArenaGroundCapture::Invalid;}
 if(admitted==ArenaEntityRegistration::UnsupportedEntity)return ArenaGroundCapture::Unsupported;
 statistics[recipient]=destination;
 if(admitted==ArenaEntityRegistration::CapacityRejected)return ArenaGroundCapture::CapacityRejected;
 statistics[owner]=source;
 // 8458..850C: retain lower category/stage bits, reset linked halfword,
 // and copy the capturing peer's target. Context owner/team follows in caller.
 victim[0x33]=static_cast<std::uint8_t>((victim[0x33]&31)|(peer[0x33]&224));
 victim[0x1e]=victim[0x1f]=255;victim[0x7e]=peer[0x7e];
 return ArenaGroundCapture::Captured;
}
}
