#include "comet/arena_construction_simulation.hpp"
#include <bit>
#include <cassert>
#include <memory>
using namespace comet::decomp;
float scalar(const ArenaEntitySlot&s,unsigned o){unsigned w=0;for(unsigned i=0;i<4;++i)w=(w<<8)|s[o+i];return std::bit_cast<float>(w);}
int main(){
 ArenaConstructionSimulationConfig c;c.capacity=32;c.seed=4;c.ground_transport_enabled=true;c.ground_contacts_enabled=true;
 c.players={{{{0,0},0,false},100,100},{{{12,12},1,false},100,100}};
 c.unit_base_available[1]=true; // Stationary enemy fighters have no available target.
 c.unit_quotas[0].capacity=1;c.unit_quotas[0].pending=c.unit_quotas[0].by_type[4]=1;
 c.unit_quotas[1].capacity=16;c.unit_quotas[1].pending=c.unit_quotas[1].by_type[1]=16;
 auto world=std::make_unique<ArenaConstructionSimulation>(c);
 assert(world->spawn_reserved_ground_unit(0,44,{3,0,3,0}).entity_index==0);
 for(unsigned i=0;i<16;++i){const float p=3.f+float(i)*.45f;assert(world->spawn_reserved_ground_unit(1,41,{p,0,p,0}).entity_index);}
 auto checkpoint=std::make_unique<ArenaConstructionSimulation>(*world);
 bool hopped=false,slammed=false,damaged=false;
 std::vector<std::vector<std::uint16_t>> emissions;
 for(unsigned n=1;n<=800;++n){
  const auto tick=world->advance(float(n)*.025f);
  emissions.push_back(tick.ground_slams);
  const auto &bank=world->storage().banks[0];
  hopped|=scalar(bank[0],4)>.3f;
  slammed|=!tick.ground_slams.empty();
  for(unsigned i=1;i<=16;++i)damaged|=scalar(bank[i],0x38)<170.f;
 }
 assert(hopped && slammed && damaged); // No injected readiness, velocity or health delta.
 assert(world->storage().banks[0]==world->storage().banks[1]);
 for(unsigned n=1;n<=800;++n)assert(checkpoint->advance(float(n)*.025f).ground_slams==emissions[n-1]);
 assert(checkpoint->storage().banks==world->storage().banks);
 assert(checkpoint->storage().cells==world->storage().cells);
 assert(checkpoint->player_statistics()==world->player_statistics());
}
