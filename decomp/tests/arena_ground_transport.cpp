#include "comet/arena_ground_transport.hpp"
#include "comet/arena_unit_spawn.hpp"
#include "comet/native_construction_scene.hpp"
#include <bit>
#include <cassert>
#include <cfenv>
#include <cmath>
#include <limits>
using namespace comet::decomp;
float scalar(const ArenaEntitySlot &s, unsigned o) {
  unsigned v=0; for(unsigned i=0;i<4;++i) v=(v<<8)|s[o+i];
  return std::bit_cast<float>(v);
}
struct Fixture {
  ArenaEntityStorage storage;
  ArenaRoutingGrid grid;
  std::array<ArenaUnitTargetPlayer,2> players{{{0,true,255},{1,true,255}}};
  ArenaRandom random{1};
  Fixture(unsigned kind=40) {
    for(auto &b:storage.banks)b.resize(1);
    reset_arena_entity_index_pool(storage,1);
    for(auto &c:grid.cells)c.routes.fill(14); // east
    std::array<std::uint8_t,128> stats{};
    ArenaUnitProductionQuota q; q.capacity=1;q.pending=1;q.by_type[kind-40]=1;
    store_arena_unit_production_quota(stats,q);
    assert(spawn_reserved_arena_ground_unit(storage,stats,kind,0,
      {2.95f,0,3.5f,0},players,grid,random).status==ArenaUnitSpawnStatus::Spawned);
  }
};
void put(ArenaEntitySlot &s,unsigned o,unsigned bits) {
  for(unsigned i=0;i<4;++i)s[o+i]=bits>>(24-8*i);
}
int main() {
  {
    ArenaConstructionSimulationConfig c;c.capacity=2;c.native_extent=8;
    c.players={{{{0,0},0,false},400,400}};
    c.tuning[26]=ArenaStructureTableValues{100,50,10,20,7.5f};
    ArenaConstructionSimulation sim(c);
    assert(sim.enqueue({0,26,8,0})==ArenaConstructionSubmit::InvalidRequest);
    assert(sim.enqueue_event(0,{1,8,0,0,26})==ArenaTimedSubmit::Invalid);
    assert(sim.spawn_reserved_ground_unit(0,40,{8,0,0,0}).status==ArenaUnitSpawnStatus::Invalid);
  }
  {
    Fixture f;for(auto &c:f.grid.cells)c.routes.fill(6); // west
    for(auto &b:f.storage.banks)put(b[0],0,0x402d0e56);
    const auto result=transport_arena_ground_unit(f.storage,0,
      std::bit_cast<float>(0x3df9db22u),f.players,f.grid,f.random);
    assert(result==ArenaGroundTransport::Moved);
    // Original FMA: 2D0E56 + (-1.5999999)*0.121999993, toward zero.
    // Separate multiply/add produces 4020902E and must fail this check.
    assert(std::bit_cast<unsigned>(scalar(f.storage.banks[0][0],0))==0x4020902d);
  }
  {
    Fixture f(43);for(auto &s:f.storage.banks)put(s[0],0,std::bit_cast<unsigned>(2.99f));
    f.grid.cells[3*24+3].routes[0]=1; // intermediate cell; endpoint is cell 4
    const auto before=f.storage.banks;
    assert(transport_arena_ground_unit(f.storage,0,.25f,f.players,f.grid,f.random)==ArenaGroundTransport::Blocked);
    assert(f.storage.banks==before);
  }
  {
    Fixture f;for(auto &c:f.grid.cells)c.routes.fill(0); // northeast diagonal
    for(auto &s:f.storage.banks){put(s[0],0,std::bit_cast<unsigned>(2.99f));put(s[0],8,std::bit_cast<unsigned>(3.99f));}
    f.grid.cells[3*24+3].routes[0]=1; // side cell at the diagonal corner
    const auto before=f.storage.banks;
    assert(transport_arena_ground_unit(f.storage,0,.25f,f.players,f.grid,f.random)==ArenaGroundTransport::Blocked);
    assert(f.storage.banks==before);
  }
  {
    ArenaConstructionSimulationConfig config;
    config.capacity=4;config.players={{{{0,0},0,false},400,400},{{{10,10},1,false},400,400}};
    config.tuning[26]=ArenaStructureTableValues{100,50,10,20,7.5f};
    config.production_enabled=true;config.unit_quotas[0].capacity=2;
    config.unit_base_available={true,true,false,false};
    ArenaState arena;arena.extent=24;arena.players.resize(2);
    arena.players[0].start_grid=ArenaGridPosition{0,0};
    arena.players[1].start_grid=ArenaGridPosition{10,10};
    NativeConstructionScene scene(arena,config);
    assert(scene.enqueue({0,26,2,2})==ArenaConstructionSubmit::Queued);
    scene.step();scene.advance(2);scene.advance(4);
    assert(scene.spawn_reserved_ground_unit(0,40,{3,3}).entity_index==1);
    const auto p=scene.arena().entities[1].position;
    scene.advance(5);assert(scene.arena().entities[1].position.x==p.x);
    scene.set_ground_transport(true);
    const auto tick=scene.advance(7);
    assert(!tick.transported_units.empty());
    assert(scene.arena().entities[1].position.x!=p.x || scene.arena().entities[1].position.z!=p.z);
    assert(scene.simulation().storage().banks[0]==scene.simulation().storage().banks[1]);
    const auto moved=scene.arena().entities[1].position;
    scene.set_ground_transport(false);scene.advance(8);
    assert(scene.arena().entities[1].position.x==moved.x && scene.arena().entities[1].position.z==moved.z);
    assert(scene.arena().cells[2*24+2].entity_index==0);
  }
  for(unsigned kind:{40u,41u,43u,44u,45u,46u,47u}) {
    Fixture f(kind); const auto before=f.storage.banks[0][0]; auto rng=f.random;
    const float x=scalar(before,0), y=scalar(before,4);
    assert(transport_arena_ground_unit(f.storage,0,.25f,f.players,f.grid,f.random)
      ==ArenaGroundTransport::Moved);
    const auto &s=f.storage.banks[0][0];
    assert(scalar(s,0)>x && scalar(s,4)==y && f.storage.banks[0]==f.storage.banks[1]);
    assert(f.random.next()==rng.next()); // valid target stays selected
    const unsigned cell=(s[0xca]*256+s[0xcb])*24+s[0xc8]*256+s[0xc9];
    assert(f.storage.cells[0][cell].size()==1 && f.storage.cells[0]==f.storage.cells[1]);
    assert(f.storage.summary_counts[cell]==1 && f.storage.free_indices.empty());
    for(unsigned i=0;i<256;++i)
      if(i>=12 && !(i>=0x20&&i<0x30) && !(i>=0x40&&i<0x50) && i!=0x7e && !(i>=0xc8&&i<0xcc))
        assert(s[i]==before[i]);
  }
  Fixture blocked; for(auto &c:blocked.grid.cells)c.routes.fill(1);
  const auto bank=blocked.storage.banks;
  const auto lists=blocked.storage.cells;
  assert(transport_arena_ground_unit(blocked.storage,0,.1f,blocked.players,blocked.grid,blocked.random)==ArenaGroundTransport::Blocked);
  assert(blocked.storage.banks==bank && blocked.storage.cells==lists);
  for(float delta:{0.f,-1.f,std::numeric_limits<float>::quiet_NaN(),1.f}) {
    assert(transport_arena_ground_unit(blocked.storage,0,delta,blocked.players,blocked.grid,blocked.random)==ArenaGroundTransport::Invalid);
    assert(blocked.storage.banks==bank);
  }
  Fixture no_target; no_target.players[1].base_available=false;
  assert(transport_arena_ground_unit(no_target.storage,0,.1f,no_target.players,no_target.grid,no_target.random)==ArenaGroundTransport::NoTarget);
  Fixture full;
  for(auto &b:full.storage.cells)b[3*24+3].resize(126);
  const auto full_before=full.storage.banks;
  assert(transport_arena_ground_unit(full.storage,0,.25f,full.players,full.grid,full.random)==ArenaGroundTransport::CellFull);
  assert(full.storage.banks==full_before);
  const int rounding=std::fegetround(); std::fesetround(FE_UPWARD);
  Fixture r; transport_arena_ground_unit(r.storage,0,.1f,r.players,r.grid,r.random);
  assert(std::fegetround()==FE_UPWARD); std::fesetround(rounding);
}
