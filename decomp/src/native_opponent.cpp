#include "comet/arena_construction_simulation.hpp"
#include <algorithm>
#include <cstdlib>
#include <tuple>

namespace comet::decomp {
bool ArenaConstructionSimulation::native_opponent_active(std::size_t owner,
    const std::array<std::array<std::uint8_t,128>,4>& statistics) const {
  auto alive=[&](std::size_t p) {
    // Base impacts consume a nonnegative counter. Absent counters keep preview bases active.
    return !routes_[p].inactive && config_.unit_base_available[p] &&
      (!config_.base_counters[p] || (statistics[p][88]!=0 || statistics[p][89]!=0));
  };
  if(!alive(owner))return false;
  for(unsigned other=0;other<players_.size();++other)
    if(alive(other) && routes_[other].team!=routes_[owner].team)return true;
  return false;
}
// One-second decisions, one ordinary factory, default fighters, held release.
// Every placement still passes the recovered resource, occupancy and route gates.
void ArenaConstructionSimulation::advance_native_opponents() {
  for(unsigned owner=0;owner<players_.size();++owner) {
    if(!config_.native_opponents[owner])continue;
    if(!native_opponent_active(owner,statistics_)) {
      config_.ground_deployment[owner]=ArenaGroundDeploymentControl{40,0};
      continue;
    }
    if(clock_<opponent_decision_times_[owner])continue;
    opponent_decision_times_[owner]=clock_+1;
    config_.ground_deployment[owner]=ArenaGroundDeploymentControl{40,1};
    // Respect explicitly scheduled map commands and global queue backpressure.
    if(players_[owner].dispatch_blocked || players_[owner].command_mode!=11 ||
       !players_[owner].events.empty())continue;
    const auto &bank=storage_.banks[storage_.read_bank];
    bool factory=false;
    for(unsigned i=0;i<bank.size();++i)
      if(bank[i][15]==26 && (bank[i][0x33]>>5)==owner) {
        factory=true;
        select_factory(static_cast<std::uint16_t>(i),4);
      }
    if(factory || !queue_.empty())continue;
    std::vector<ArenaGridPosition> candidates;
    for(unsigned z=0;z<config_.native_extent;++z)
      for(unsigned x=0;x<config_.native_extent;++x) {
        ArenaGridPosition cell{static_cast<std::uint8_t>(x),static_cast<std::uint8_t>(z)};
        if(std::none_of(routes_.begin(),routes_.end(),[&](const auto &r){return r.base==cell;}))
          candidates.push_back(cell);
      }
    const auto base=routes_[owner].base;
    auto key=[&](ArenaGridPosition p) {
      const int dx=int(p.x)-base.x,dz=int(p.z)-base.z;
      return std::tuple{dx*dx+dz*dz,p.z,p.x};
    };
    std::sort(candidates.begin(),candidates.end(),[&](auto a,auto b){return key(a)<key(b);});
    for(std::size_t attempt=0;attempt<candidates.size();++attempt) {
      auto &cursor=opponent_build_cursors_[owner];
      const auto cell=candidates[cursor++ % candidates.size()];
      const auto result=enqueue({static_cast<std::uint8_t>(owner),26,cell.x,cell.z});
      if(result==ArenaConstructionSubmit::Queued ||
         result==ArenaConstructionSubmit::InsufficientResources ||
         result==ArenaConstructionSubmit::NoEntityCapacity ||
         result==ArenaConstructionSubmit::QueueFull)break;
    }
  }
}
} // namespace comet::decomp
