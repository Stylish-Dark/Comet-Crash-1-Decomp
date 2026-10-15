#include "comet/native_deployment_input.hpp"
#include <cassert>
#include <memory>
#include <string>
using namespace comet::decomp;
int main(){
 ArenaConstructionSimulationConfig c;c.capacity=8;c.seed=4;
 c.players={{{{1,1},0,false},100,100},{{{4,1},1,false},100,100}};
 c.unit_base_available={true,true,false,false};c.ground_transport_enabled=true;c.base_counters[1]=2;
 c.unit_quotas[0].capacity=3;c.unit_quotas[0].pending=3;c.unit_quotas[0].by_type[0]=2;c.unit_quotas[0].by_type[1]=1;
 ArenaState arena;arena.players.resize(2);arena.players[0].start_grid=ArenaGridPosition{1,1};arena.players[1].start_grid=ArenaGridPosition{4,1};
 auto scene=std::make_unique<NativeConstructionScene>(arena,c);
 {
  NativeDeploymentInput input(*scene);
  assert(input.owner()==0 && input.kind()==40);
  scene->advance(.1f);assert(scene->simulation().unit_quotas()[0].active==0);
  input.press();scene->advance(.2f);
  assert(scene->simulation().unit_quotas()[0].active==1 && scene->simulation().unit_quotas()[0].pending==2);
  input.release();scene->advance(.3f);assert(scene->simulation().unit_quotas()[0].active==1);
  input.cycle_kind(1);assert(input.kind()==41);input.press();scene->advance(.4f);
  assert(scene->simulation().unit_quotas()[0].active==2 && scene->simulation().unit_quotas()[0].by_type[1]==0);
  input.next_owner();assert(input.owner()==1 && input.kind()==40 && !input.held());
  assert(scene->simulation().ground_deployment()[0]->strength==0);
  input.cycle_kind(-1);assert(input.kind()==47);input.cycle_kind(1);assert(input.kind()==40);
  input.next_owner();assert(input.kind()==41);input.cycle_kind(-1);input.press();input.cancel();
  scene->advance(.5f);assert(scene->simulation().unit_quotas()[0].pending==1);
  input.press();input.press(); // repeated key-down is idempotent
  scene->advance(.6f);assert(scene->simulation().unit_quotas()[0].pending==0);
  const auto lines=input.status_lines(false);
  assert(lines[0].find("PLAYER 1")!=std::string::npos && lines[0].find("PENDING 0")!=std::string::npos);
  assert(lines[1].find("TYPE 40")!=std::string::npos && lines[1].find("HELD")!=std::string::npos);
  assert(lines[2].find("2:2")!=std::string::npos && lines[2].find("1:--")!=std::string::npos);
 }
 assert(scene->simulation().ground_deployment()[0]->strength==0); // exit/error cleanup
 // Configuration-driven releases keep working until manually controlled.
 c.ground_deployment[0]=ArenaGroundDeploymentControl{45,.5f};
 auto configured=std::make_unique<NativeConstructionScene>(arena,c);
 {NativeDeploymentInput input(*configured);assert(input.kind()==45);input.cancel();assert(configured->simulation().ground_deployment()[0]->strength==.5f);}
 assert(configured->simulation().ground_deployment()[0]->strength==.5f);
}
