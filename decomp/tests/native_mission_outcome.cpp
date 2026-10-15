#include "comet/native_mission_outcome.hpp"
#include <cassert>
using namespace comet::decomp;
int main() {
 ArenaConstructionSimulationConfig c;
 c.capacity=0;
 c.players={{{{0,0},0,false},100,100},{{{3,3},1,false},100,100}};
 ArenaConstructionSimulation unconfigured(c);
 assert(evaluate_native_mission(unconfigured,0)==NativeMissionOutcome::Unconfigured);
 c.base_counters[0]=2;c.base_counters[1]=1;
 ArenaConstructionSimulation active(c);
 assert(evaluate_native_mission(active,0)==NativeMissionOutcome::InProgress);
 assert(evaluate_native_mission(active,1)==NativeMissionOutcome::InProgress);
 assert(evaluate_native_mission(active,2)==NativeMissionOutcome::Unconfigured);
 c.base_counters[1]=0;
 ArenaConstructionSimulation won(c);
 assert(evaluate_native_mission(won,0)==NativeMissionOutcome::Victory);
 assert(evaluate_native_mission(won,1)==NativeMissionOutcome::Defeat);
 c.base_counters[0]=0;
 ArenaConstructionSimulation drawn(c);
 assert(evaluate_native_mission(drawn,0)==NativeMissionOutcome::Draw);
 c.players[1].route.team=0;
 ArenaConstructionSimulation same_team(c);
 assert(evaluate_native_mission(same_team,0)==NativeMissionOutcome::Unconfigured);
}
