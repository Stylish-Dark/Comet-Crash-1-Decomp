#include "comet/native_construction_scene.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace comet::decomp {
ArenaConstructionSimulationConfig
NativeConstructionScene::bootstrap(const ArenaState &a,
                                   ArenaConstructionSimulationConfig c) {
  if (!a.extent || a.extent > 24 || a.players.size() != c.players.size() ||
      !a.unsupported_records.empty())
    throw std::invalid_argument("unsupported construction arena");
  c.native_extent = a.extent;
  c.initial_structures.clear();
  c.initial_players = a.players;
  for (std::size_t i = 0; i < c.players.size(); ++i) {
    auto &r = c.players[i].route;
    if (r.base.x >= a.extent || r.base.z >= a.extent ||
        (!r.inactive &&
         a.players[i].start_grid != std::optional<ArenaGridPosition>(r.base)))
      throw std::invalid_argument("player base differs from map");
  }
  for (const auto &player : a.players) {
    auto events = player.events;
    while (!events.empty()) {
      auto e = events.pop();
      if (e.selector < 9 && (e.grid_x >= a.extent || e.grid_z >= a.extent))
        throw std::invalid_argument("map event outside arena");
    }
    if (player.command_mode != 11 && player.pending.selector < 9 &&
        (player.pending.grid_x >= a.extent ||
         player.pending.grid_z >= a.extent))
      throw std::invalid_argument("pending command outside arena");
  }
  for (unsigned i = 0; i < 576; ++i) {
    const auto &cell = a.cells[i];
    if (cell.entity_index && *cell.entity_index >= a.entities.size())
      throw std::invalid_argument("invalid map cell entity link");
    c.initial_visibility[i] = cell.visibility;
    if (!cell.visibility_recovered) {
      if (!cell.entity_index || *cell.entity_index >= a.entities.size())
        throw std::invalid_argument("unresolved map visibility");
      const auto &e = a.entities[*cell.entity_index];
      if (e.type != 25 || e.owner >= c.players.size() ||
          e.grid.z * 24u + e.grid.x != i)
        throw std::invalid_argument("invalid gate visibility source");
      c.initial_visibility[i] =
          arena_gate_visibility(e.owner, c.players[e.owner].route.team);
    }
  }
  for (const auto &e : a.entities) {
    if (e.grid.x >= a.extent || e.grid.z >= a.extent)
      throw std::invalid_argument("entity outside arena");
    if (native_map_structure_importable(e))
      c.initial_structures.push_back(e);
  }
  return c;
}
NativeConstructionScene::NativeConstructionScene(
    const ArenaState &a, const ArenaConstructionSimulationConfig &c)
    : arena_(a), simulation_(bootstrap(a, c)) {
  arena_.entities.reserve(a.entities.size() + c.capacity);
  std::uint16_t index = 0;
  for (auto &e : arena_.entities) {
    e.live_entity_index.reset();
    e.construction_progress.reset();
    e.construction_ready = false;
    e.production_mode.reset();
    e.production_elapsed.reset();
    e.production_period.reset();
    if (native_map_structure_importable(e)) {
      e.live_entity_index = index++;
      ++imports_.live;
    } else
      ++imports_.display_only;
  }
  for (unsigned i = 0; i < 576; ++i) {
    arena_.cells[i].visibility = simulation_.grid().cells[i].visibility;
    arena_.cells[i].visibility_recovered = true;
  }
  refresh_construction();
}
ArenaConstructionSubmit
NativeConstructionScene::enqueue(ArenaConstructionRequest r) {
  if (r.grid_x >= arena_.extent || r.grid_z >= arena_.extent)
    return ArenaConstructionSubmit::InvalidRequest;
  return simulation_.enqueue(r);
}
void NativeConstructionScene::refresh_construction() {
  const auto &bank =
      simulation_.storage().banks[simulation_.storage().read_bank];
  for (auto &entity : arena_.entities) {
    if (!entity.live_entity_index)
      continue;
    const auto &slot = bank[*entity.live_entity_index];
    if (slot[15]==1 || slot[15]==3 || slot[15]==5 || (slot[15] >= 40 && slot[15] <= 48) || slot[15]==62) {
      auto scalar = [&](unsigned o) {
        std::uint32_t bits=0;
        for (unsigned i=0;i<4;++i) bits=(bits<<8)|slot[o+i];
        return std::bit_cast<float>(bits);
      };
      entity.type=slot[15];
      entity.beam_path.reset();
      if(slot[15]==5){
        std::array<ModelPosition,5> path;
        for(unsigned i=0;i<5;++i)path[i]={scalar(0x80+16*i),scalar(0x84+16*i),scalar(0x88+16*i)};
        entity.beam_path=path;
      }
      if(slot[15]==62){
        const auto initial=scalar(0x88);
        entity.corpse_fraction=initial>0?std::clamp(scalar(0x78)/initial,0.f,1.f):1.f;
      }else entity.corpse_fraction.reset();
      entity.owner=slot[0x33]>>5;
      entity.position = {scalar(0), scalar(4), scalar(8)};
      entity.grid = {static_cast<std::uint8_t>(std::clamp(std::trunc(entity.position.x),0.f,float(arena_.extent-1))),
                     static_cast<std::uint8_t>(std::clamp(std::trunc(entity.position.z),0.f,float(arena_.extent-1)))};
      for (unsigned i=0;i<4;++i) entity.orientation[i]=scalar(0x20+4*i);
      continue;
    }
    std::uint32_t word = 0;
    for (unsigned i = 0; i < 4; ++i)
      word = (word << 8) | slot[0x78 + i];
    entity.construction_progress = std::bit_cast<float>(word);
    if(slot[15]==21)for(unsigned axis=0;axis<4;++axis){
      unsigned bits=0;for(unsigned b=0;b<4;++b)bits=(bits<<8)|slot[0x20+4*axis+b];
      entity.orientation[axis]=std::bit_cast<float>(bits);
    }
    entity.construction_ready = arena_structure_ready(slot);
    if (slot[15] == 26 || slot[15] == 28) {
      entity.production_mode = slot[0xaa];
      auto scalar = [&](unsigned offset) {
        std::uint32_t bits = 0;
        for (unsigned i = 0; i < 4; ++i)
          bits = (bits << 8) | slot[offset + i];
        return std::bit_cast<float>(bits);
      };
      entity.production_elapsed = scalar(0x80);
      entity.production_period = scalar(0x84);
    }
  }
}
void NativeConstructionScene::project(
    const ArenaConstructionSimulationStep &s) {
  for (unsigned i = 0; i < 576; ++i)
    arena_.cells[i].visibility = simulation_.grid().cells[i].visibility;
  if (s.status != ArenaConstructionStep::Constructed || !s.entity_index)
    return;
  const auto &slot =
      simulation_.storage()
          .banks[simulation_.storage().read_bank][*s.entity_index];
  auto f = [&](unsigned off) {
    std::uint32_t word = 0;
    for (unsigned i = 0; i < 4; ++i)
      word = (word << 8) | slot[off + i];
    return std::bit_cast<float>(word);
  };
  ArenaEntity e;
  e.type = slot[15];
  e.owner = slot[0x33] >> 5;
  e.model_slot = slot[0x6f];
  e.initial_upgrade = slot[0x6e];
  e.grid = {s.request.grid_x, s.request.grid_z};
  e.position = {f(0), f(4), f(8)};
  for (unsigned i = 0; i < 4; ++i)
    e.orientation[i] = f(0x20 + 4 * i);
  e.live_entity_index = s.entity_index;
  e.source_record = std::numeric_limits<std::size_t>::max();
  arena_.cells[e.grid.z * 24 + e.grid.x].entity_index = arena_.entities.size();
  arena_.entities.push_back(e);
}
ArenaConstructionSimulationStep NativeConstructionScene::step() {
  auto s = simulation_.step();
  project(s);
  refresh_construction();
  arena_.players = simulation_.players();
  return s;
}
ArenaFactorySelection
NativeConstructionScene::select_factory(std::uint16_t index,
                                        std::uint8_t selector) {
  const auto result = simulation_.select_factory(index, selector);
  refresh_construction();
  return result;
}
ArenaFactorySelection NativeConstructionScene::select_factory(
    std::size_t player, ArenaGridPosition cell, std::uint8_t selector) {
  if (cell.x >= arena_.extent || cell.z >= arena_.extent)
    return ArenaFactorySelection::InvalidRequest;
  if (player >= simulation_.accounts().size())
    return ArenaFactorySelection::InvalidRequest;
  const auto *entry = arena_.cell(cell.x, cell.z);
  if (!entry || !entry->entity_index)
    return ArenaFactorySelection::NoTarget;
  const auto &entity = arena_.entities[*entry->entity_index];
  if (!entity.live_entity_index || entity.owner != player)
    return ArenaFactorySelection::NoTarget;
  return select_factory(*entity.live_entity_index, selector);
}
ArenaConstructionTick NativeConstructionScene::advance(float t) {
  auto tick = simulation_.advance(t);
  if(!tick.retired_entities.empty()){
    // Retire by the tick's identities BEFORE projecting construction: a newly
    // allocated structure can already have reused one of the retired indices.
    std::vector<std::optional<std::size_t>> remap(arena_.entities.size());
    std::size_t write=0;
    for(std::size_t old=0;old<arena_.entities.size();++old){
      const auto &e=arena_.entities[old];
      const bool retired=e.live_entity_index && std::find(tick.retired_entities.begin(),tick.retired_entities.end(),*e.live_entity_index)!=tick.retired_entities.end();
      if(retired)continue;
      remap[old]=write;
      if(write!=old)arena_.entities[write]=arena_.entities[old];
      ++write;
    }
    arena_.entities.resize(write);
    for(auto &cell:arena_.cells)if(cell.entity_index)cell.entity_index=remap[*cell.entity_index];
  }
  project(tick.construction);
  for(const auto &attempt:tick.ground_deployments)if(attempt.result.entity_index)project_dynamic_entity(*attempt.result.entity_index);
  for(auto index:tick.spawned_beams){
    const auto &bank=simulation_.storage().banks[simulation_.storage().read_bank];
    if(index<bank.size() && bank[index][15]==5 &&
       std::none_of(arena_.entities.begin(),arena_.entities.end(),[&](const auto &e){return e.live_entity_index==index;}))
      project_dynamic_entity(index);
  }
  for(auto index:tick.spawned_projectiles){
    const auto &bank=simulation_.storage().banks[simulation_.storage().read_bank];
    if(index<bank.size() && bank[index][15]==3 &&
       std::none_of(arena_.entities.begin(),arena_.entities.end(),[&](const auto&e){return e.live_entity_index==index;}))project_dynamic_entity(index);
  }
  refresh_construction();
  arena_.players = simulation_.players();
  return tick;
}
ArenaCraftRegistration NativeConstructionScene::create_player_craft(std::uint8_t owner,
    const std::array<float,4>& spawn,const ArenaEntitySlot& reused,const ArenaCraftAuxiliaryStart& auxiliary){
  const auto result=simulation_.create_player_craft(owner,spawn,reused,auxiliary);
  if(result==ArenaCraftRegistration::Registered){
    project_dynamic_entity(*simulation_.craft_registry().indices[owner]);
    arena_.players=simulation_.players();
  }
  return result;
}
ArenaPlayerCraftControlStep NativeConstructionScene::advance_prepared_player_craft(std::uint8_t owner,
    std::span<const std::uint8_t,0x2900> snapshot,float elapsed,ArenaEntityWorkContext& work,
    std::uint32_t frame,ArenaPlayerControlScratch* scratch,const ArenaPlayerPendingTransport& pending,
    const ArenaCounterEmissionSink& emitter){
  const auto result=simulation_.advance_prepared_player_craft(owner,snapshot,elapsed,work,frame,scratch,pending,emitter);
  if(result.control.status==ArenaPlayerControlStatus::Advanced)refresh_construction();
  return result;
}
ArenaEntityFinalization NativeConstructionScene::finalize_player_craft(std::uint8_t owner,
    const ArenaEntitySlot& updated,const ArenaEntityFinalizationContext& context,
    std::uint32_t frame){
  const auto result=simulation_.finalize_player_craft(owner,updated,context,frame);
  if(result==ArenaEntityFinalization::Committed)refresh_construction();
  return result;
}
ArenaUnitSpawnResult NativeConstructionScene::spawn_reserved_ground_unit(
    std::uint8_t owner, std::uint8_t kind, ArenaGridPosition cell) {
  if (cell.x >= arena_.extent || cell.z >= arena_.extent)
    return {};
  const auto result = simulation_.spawn_reserved_ground_unit(
      owner, kind, {cell.x + .5f, 0, cell.z + .5f, 0});
  if(result.entity_index)project_dynamic_entity(*result.entity_index);
  return result;
}
void NativeConstructionScene::project_dynamic_entity(std::uint16_t index){
  const auto &slot =
      simulation_.storage()
          .banks[simulation_.storage().read_bank][index];
  auto scalar = [&](unsigned o) {
    std::uint32_t bits = 0;
    for (unsigned i = 0; i < 4; ++i)
      bits = (bits << 8) | slot[o + i];
    return std::bit_cast<float>(bits);
  };
  ArenaEntity e;
  e.type = slot[15];
  e.owner = slot[0x33] >> 5;
  e.model_slot = slot[0x6f];
  e.initial_upgrade = slot[0x6e];
  e.position = {scalar(0), scalar(4), scalar(8)};
  e.grid = {static_cast<std::uint8_t>(
                std::clamp(std::trunc(e.position.x), 0.0f,
                           static_cast<float>(arena_.extent - 1))),
            static_cast<std::uint8_t>(
                std::clamp(std::trunc(e.position.z), 0.0f,
                           static_cast<float>(arena_.extent - 1)))};
  for (unsigned i = 0; i < 4; ++i)
    e.orientation[i] = scalar(0x20 + 4 * i);
  e.live_entity_index = index;
  e.source_record = std::numeric_limits<std::size_t>::max();
  // Cell.entity_index remains the structure lookup link; mobile records live
  // in the simulation's ordered cell lists and may share a structure cell.
  arena_.entities.push_back(e);
}
} // namespace comet::decomp
