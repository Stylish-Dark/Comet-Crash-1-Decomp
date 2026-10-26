#include "comet/arena_construction_simulation.hpp"
#include "comet/arena_ground_base.hpp"
#include "comet/arena_ground_lifecycle.hpp"
#include "comet/arena_health_delta.hpp"
#include "comet/arena_converter_activation.hpp"
#include "comet/arena_ground_slam_effect.hpp"
#include "comet/arena_ground_slam.hpp"
#include "arena_tesla_runtime.hpp"
#include "arena_cannon_runtime.hpp"
#include "comet/arena_impulse.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
bool supported(unsigned type) {
  return (type >= 20 && type <= 26) || type == 28;
}
bool factory_command(unsigned selector, unsigned type) {
  return selector >= 4 && selector <= 8 && (type == 26 || type == 28);
}
void scalar(ArenaEntitySlot &s, unsigned off, float f) {
  auto word = std::bit_cast<std::uint32_t>(f);
  for (unsigned i = 0; i < 4; ++i)
    s[off + i] = word >> (24 - i * 8);
}
} // namespace
bool native_map_structure_importable(const ArenaEntity &e) {
  auto level = arena_structure_initial_level(e.type);
  return level && e.initial_upgrade <= *level;
}
ArenaConstructionSimulation::ArenaConstructionSimulation(
    const ArenaConstructionSimulationConfig &c)
    : config_(c), random_(c.seed) {
  if(config_.ground_contacts_enabled || config_.cannon_enabled)config_.ground_dynamics_enabled=true;
  if (c.players.empty() || c.players.size() > 4 || c.capacity > 65536 || !c.native_extent || c.native_extent > 24)
    throw std::invalid_argument("invalid simulation dimensions");
  health_deltas_.resize(c.capacity);
  impulse_deltas_.resize(c.capacity);
  tesla_commands_.resize(c.capacity);
  if((c.tesla_enabled || c.cannon_enabled) && c.capacity>65535)throw std::invalid_argument("weapon links reserve index 65535");
  for (const auto &a : c.players) {
    if (a.route.base.x >= c.native_extent || a.route.base.z >= c.native_extent || a.route.team > 3 ||
        !std::isfinite(a.resource) || !std::isfinite(a.resource_max) ||
        a.resource < 0 || a.resource_max < a.resource)
      throw std::invalid_argument("invalid player account");
    routes_.push_back(a.route);
    ArenaPlayer p;
    p.start_grid = a.route.base;
    players_.push_back(p);
  }
  for(unsigned owner=0;owner<4;++owner)if(c.ground_deployment[owner]) {
    if(!set_ground_deployment(owner,*c.ground_deployment[owner]))
      throw std::invalid_argument("invalid ground deployment control");
  }
  for(unsigned owner=0;owner<4;++owner)if(c.native_opponents[owner]) {
    if(owner>=c.players.size() || c.players[owner].route.inactive ||
       !c.production_enabled || !c.ground_transport_enabled ||
       !c.unit_base_available[owner] || !c.unit_quotas[owner].capacity ||
       !c.tuning[26] || c.ground_deployment[owner])
      throw std::invalid_argument("native opponent requires factory tuning, production, transport, base and quota; deployment is automatic");
    config_.ground_deployment[owner]=ArenaGroundDeploymentControl{40,0};
  }
  for(unsigned owner=0;owner<4;++owner)if(c.base_counters[owner]) {
    if(owner>=c.players.size() || *c.base_counters[owner]<0)
      throw std::invalid_argument("invalid base counter");
    const auto value=static_cast<std::uint16_t>(*c.base_counters[owner]);
    statistics_[owner][88]=value>>8;statistics_[owner][89]=value;
  }
  for (unsigned owner = 0; owner < c.players.size(); ++owner) {
    const auto forced = c.unit_target_overrides[owner];
    if (forced != 255 && (forced >= c.players.size() || forced == owner))
      throw std::invalid_argument("invalid unit target override");
    const auto &q = c.unit_quotas[owner];
    std::uint32_t total = 0;
    for (auto count : q.by_type)
      total += count;
    if (q.active < 0 || q.capacity > 65535 || total != q.pending ||
        static_cast<std::uint32_t>(q.active) + q.pending > q.capacity)
      throw std::invalid_argument("invalid production quota");
    store_arena_unit_production_quota(statistics_[owner], q);
  }
  if (!c.initial_players.empty()) {
    if (c.initial_players.size() != players_.size())
      throw std::invalid_argument("event player count mismatch");
    players_ = c.initial_players;
    for (const auto &p : players_) {
      auto events = p.events;
      if (events.size() > p.event_capacity)
        throw std::invalid_argument("event heap exceeds capacity");
      while (!events.empty()) {
        auto e = events.pop();
        if (!std::isfinite(e.time) ||
            (e.selector < 9 && (e.grid_x >= c.native_extent || e.grid_z >= c.native_extent)))
          throw std::invalid_argument("invalid event cell");
      }
      if (p.command_mode != 11 &&
          (p.pending.selector != p.command_mode ||
           (p.pending.selector < 9 &&
            (p.pending.grid_x >= c.native_extent || p.pending.grid_z >= c.native_extent))))
        throw std::invalid_argument("invalid pending command");
    }
  }
  for (unsigned op = 0; op < 32; ++op) {
    const auto &t = c.tuning[op];
    if (t && (!supported(op) || t->scalar_38_denominator <= 0 ||
              !std::isfinite(t->scalar_84)))
      throw std::invalid_argument("invalid tuning");
  }
  if (c.initial_structures.size() > c.capacity)
    throw std::invalid_argument("map exceeds capacity");
  std::array<unsigned, 576> counts{};
  auto visibility = c.initial_visibility;
  for (const auto &e : c.initial_structures) {
    if (!native_map_structure_importable(e) || e.owner >= players_.size() ||
        e.grid.x >= c.native_extent || e.grid.z >= c.native_extent || !c.tuning[e.type] ||
        e.model_slot != *initial_model_slot_for_record(e.type) ||
        e.position.x != e.grid.x + .5f || e.position.y != 0 ||
        e.position.z != e.grid.z + .5f ||
        ++counts[e.grid.z * 24 + e.grid.x] > 126)
      throw std::invalid_argument("invalid initial structure");
    for (float f : e.orientation)
      if (!std::isfinite(f))
        throw std::invalid_argument("invalid orientation");
    auto flat = e.grid.z * 24 + e.grid.x;
    auto supplied = c.initial_visibility[flat];
    if (supplied && !(supplied & 0xf0))
      throw std::invalid_argument("unowned initial occupancy");
    if (!supplied)
      visibility[flat] =
          e.type == 25 ? arena_gate_visibility(e.owner, routes_[e.owner].team)
                       : static_cast<std::uint8_t>(((e.owner + 1) << 4) | 15);
  }
  if (!initialize_arena_routing_grid(visibility, routes_, grid_))
    throw std::invalid_argument("initial grid blocks routes");
  storage_.width = storage_.height = c.native_extent;
  for (auto &bank : storage_.banks)
    bank.resize(c.capacity);
  reset_arena_entity_index_pool(storage_, c.capacity);
  for (const auto &e : c.initial_structures) {
    ArenaEntitySlot slot{};
    if (!initialize_arena_structure_entity(slot, e.type, e.owner, e.grid,
                                           *c.tuning[e.type], &random_))
      throw std::invalid_argument("constructor failed");
    for (unsigned i = 0; i < 4; ++i)
      scalar(slot, 0x20 + 4 * i, e.orientation[i]);
    if (!settle_arena_map_structure(slot, e.initial_upgrade, e.variant) ||
        !insert_arena_entity(storage_, e.grid, slot))
      throw std::invalid_argument("map import failed");
  }
}
ArenaCraftRegistration ArenaConstructionSimulation::create_player_craft(
    std::uint8_t owner,const std::array<float,4>& spawn,
    const ArenaEntitySlot& reused_slot,const ArenaCraftAuxiliaryStart& auxiliary) {
  if(owner>=players_.size() || spawn[1]!=0)return ArenaCraftRegistration::Invalid;
  const auto resolved=resolve_arena_player_craft_spawn(owner,storage_.width,
      storage_.height,spawn[0],spawn[2],spawn[3]);
  if(!resolved)return ArenaCraftRegistration::Invalid;
  return register_arena_player_craft(storage_,players_[owner],craft_registry_,
      owner,*resolved,reused_slot,auxiliary);
}
bool ArenaConstructionSimulation::stage_player_craft_preparation(
    std::uint8_t owner,std::span<const std::uint8_t,0x2900> snapshot,
    ArenaEntityWorkContext& context) const {
  if(owner>=players_.size() || storage_.read_bank>1 ||
     !players_[owner].craft_entity_index ||
     players_[owner].craft_entity_index!=craft_registry_.indices[owner])return false;
  const auto index=*players_[owner].craft_entity_index;
  if(index>=storage_.banks[storage_.read_bank].size())return false;
  const auto& body=storage_.banks[storage_.read_bank][index];
  if(body[15]!=1 || (body[0x33]>>5)!=owner)return false;
  auto read=[&](unsigned o){return std::uint32_t(snapshot[o])<<24|
      std::uint32_t(snapshot[o+1])<<16|std::uint32_t(snapshot[o+2])<<8|snapshot[o+3];};
  if(read(0x28d8)!=storage_.width || read(0x28dc)!=storage_.height)return false;
  auto next=context;std::copy(body.begin(),body.end(),next.begin());
  if(!stage_arena_entity_preparation(next,snapshot))return false;
  context=next;return true;
}
bool ArenaConstructionSimulation::stage_player_craft_preparation(std::uint8_t owner,
    std::span<const std::uint8_t,0x2900> snapshot,ArenaEntityWorkContext& context,
    ArenaPlayerReferenceCache& cache,std::span<const ArenaAccumulatorRecord> accumulators,
    std::uint32_t tag) const {
  auto next=context;auto next_cache=cache;
  // These pure staging writes are disjoint from B500's fetch destinations.
  // Keep all writes transactional if either source boundary is unavailable.
  if(!stage_player_craft_preparation(owner,snapshot,next)||
     !fetch_arena_player_references(next,next_cache,storage_.banks[storage_.read_bank],accumulators,tag))return false;
  context=next;cache=next_cache;return true;
}
ArenaPreparationCompletion ArenaConstructionSimulation::complete_player_craft_preparation(
    std::uint8_t owner,std::span<const std::uint8_t,0x2900> snapshot,
    const ArenaAccumulatorConsumption& consumed,ArenaEntityWorkContext& context,
    ArenaEmissionDescriptor* scratch,const ArenaCounterEmissionSink& emitter) {
  if(owner>=players_.size() || storage_.read_bank>1 ||
     !players_[owner].craft_entity_index ||
     players_[owner].craft_entity_index!=craft_registry_.indices[owner] ||
     context[15]!=1 || (context[0x33]>>5)!=owner ||
     (unsigned(context[0x542])<<8|context[0x543])!=owner)
    return ArenaPreparationCompletion::Invalid;
  const auto index=*players_[owner].craft_entity_index;
  if(index>=storage_.banks[storage_.read_bank].size() ||
     storage_.banks[storage_.read_bank][index][15]!=1 ||
     (unsigned(context[12])<<8|context[13])!=index)
    return ArenaPreparationCompletion::Invalid;
  auto read=[&](unsigned o){return std::uint32_t(snapshot[o])<<24|
      std::uint32_t(snapshot[o+1])<<16|std::uint32_t(snapshot[o+2])<<8|snapshot[o+3];};
  if(read(0x28d8)!=storage_.width || read(0x28dc)!=storage_.height)
    return ArenaPreparationCompletion::Invalid;
  return complete_arena_entity_preparation(context,snapshot,consumed,statistics_[owner],scratch,emitter);
}
ArenaEntityFinalization ArenaConstructionSimulation::finalize_player_craft(
    std::uint8_t owner,const ArenaEntitySlot& updated,
    const ArenaEntityFinalizationContext& context,std::uint32_t frame) {
  if(owner>=players_.size() || context.statistics_owner_542!=owner ||
     !players_[owner].craft_entity_index ||
     players_[owner].craft_entity_index!=craft_registry_.indices[owner] ||
     updated[15]!=1 || (updated[0x33]>>5)!=owner)
    return ArenaEntityFinalization::Invalid;
  const auto index=*players_[owner].craft_entity_index;
  if(index>=storage_.banks[storage_.read_bank].size() ||
     storage_.banks[storage_.read_bank][index][15]!=1)
    return ArenaEntityFinalization::Invalid;
  return finalize_arena_entity_update(storage_,index,updated,statistics_[owner],context,frame);
}
ArenaPlayerCraftControlStep ArenaConstructionSimulation::advance_prepared_player_craft(
    std::uint8_t owner,std::span<const std::uint8_t,0x2900> snapshot,float elapsed,
    ArenaEntityWorkContext& work,std::uint32_t frame,ArenaPlayerControlScratch* scratch,
    const ArenaPlayerPendingTransport& pending,const ArenaCounterEmissionSink& emitter,const ArenaPlayerParticleTransport& particles){
  ArenaPlayerCraftControlStep result;
  auto read=[&](const auto& b,unsigned o){return std::uint32_t(b[o])<<24|
      std::uint32_t(b[o+1])<<16|std::uint32_t(b[o+2])<<8|b[o+3];};
  if(owner>=players_.size()||storage_.read_bank>1||!players_[owner].craft_entity_index||
     players_[owner].craft_entity_index!=craft_registry_.indices[owner]||
     work[15]!=1||(work[0x33]>>5)!=owner||
     (unsigned(work[0x542])<<8|work[0x543])!=owner||
     read(snapshot,0x28d8)!=storage_.width||read(snapshot,0x28dc)!=storage_.height)return result;
  const auto index=*players_[owner].craft_entity_index;
  if(index>=storage_.banks[storage_.read_bank].size()||
     storage_.banks[storage_.read_bank][index][15]!=1||
     (unsigned(work[12])<<8|work[13])!=index)return result;
  result.control=advance_arena_player_control(work,snapshot,elapsed,scratch,pending,emitter,particles);
  if(result.control.status!=ArenaPlayerControlStatus::Advanced)return result;
  auto scalar=[&](unsigned o){return std::bit_cast<float>(read(work,o));};
  for(unsigned i=0;i<4;++i){result.commit.position_480[i]=scalar(0x480+4*i);result.commit.velocity_4a0[i]=scalar(0x4a0+4*i);}
  result.commit.previous_scalar_38_4e8=scalar(0x4e8);result.commit.statistics_owner_542=owner;
  result.commit.removal_590=work[0x590];result.commit.removal_handled_593=work[0x593];
  result.commit.auxiliary_596=work[0x596];result.commit.changed_59a=work[0x59a];
  ArenaEntitySlot body;std::copy_n(work.begin(),256,body.begin());
  result.finalization=finalize_player_craft(owner,body,result.commit,frame);
  if(result.finalization!=ArenaEntityFinalization::Committed){
    const auto pc=result.finalization==ArenaEntityFinalization::NeedsChangeEffects?0x3940u:
        result.finalization==ArenaEntityFinalization::NeedsSpatialRecovery?0x6260u:0x6578u;
    result.control={ArenaPlayerControlStatus::NeedsFinalization,pc};
  }
  return result;
}
ArenaConstructionContext
ArenaConstructionSimulation::context_for(ArenaConstructionRequest r) const {
  auto flat = r.grid_z * 24 + r.grid_x;
  ArenaConstructionContext c;
  c.visibility = grid_.cells[flat].visibility;
  c.cell_entry_count =
      std::max(storage_.cells[0][flat].size(), storage_.cells[1][flat].size());
  c.available_entities = storage_.free_indices.size();
  c.resource = config_.players[r.player].resource;
  c.resource_max = config_.players[r.player].resource_max;
  return c;
}
ArenaConstructionSubmit
ArenaConstructionSimulation::enqueue(ArenaConstructionRequest r) {
  if (r.player >= players_.size() || r.grid_x >= config_.native_extent || r.grid_z >= config_.native_extent ||
      !supported(r.opcode) || !config_.tuning[r.opcode])
    return ArenaConstructionSubmit::InvalidRequest;
  switch (validate_arena_construction(r, context_for(r), queue_)) {
  case ArenaConstructionError::InvalidRequest:
    return ArenaConstructionSubmit::InvalidRequest;
  case ArenaConstructionError::Occupied:
    return ArenaConstructionSubmit::Occupied;
  case ArenaConstructionError::CellFull:
    return ArenaConstructionSubmit::CellFull;
  case ArenaConstructionError::NoEntityCapacity:
    return ArenaConstructionSubmit::NoEntityCapacity;
  case ArenaConstructionError::Duplicate:
    return ArenaConstructionSubmit::Duplicate;
  case ArenaConstructionError::InsufficientResources:
    return ArenaConstructionSubmit::InsufficientResources;
  case ArenaConstructionError::None:
    break;
  }
  return queue_.push(r) ? ArenaConstructionSubmit::Queued
                        : ArenaConstructionSubmit::QueueFull;
}
ArenaConstructionSimulationStep ArenaConstructionSimulation::step() {
  if (queue_.empty())
    return {};
  ArenaConstructionSimulationStep s;
  auto preview = queue_;
  s.request = preview.pop();
  auto c = context_for(s.request);
  s.status = process_next_arena_construction(
      queue_, grid_, routes_, players_, c, [&](ArenaConstructionRequest r) {
        ArenaEntitySlot slot{};
        auto next_random = random_;
        if (!initialize_arena_structure_entity(
                slot, r.opcode, r.player, {r.grid_x, r.grid_z},
                *config_.tuning[r.opcode], &next_random))
          return false;
        s.entity_index =
            insert_arena_entity(storage_, {r.grid_x, r.grid_z}, slot);
        if (!s.entity_index)
          return false;
        random_ = next_random;
        return true;
      });
  config_.players[s.request.player].resource = c.resource;
  return s;
}
ArenaFactorySelection
ArenaConstructionSimulation::select_factory_index(std::int32_t index,
                                                  std::uint8_t selector) {
  if (index < 0 || static_cast<std::size_t>(index) >=
                       storage_.banks[storage_.read_bank].size())
    return ArenaFactorySelection::NoTarget;
  auto slot = storage_.banks[storage_.read_bank][index];
  auto result = select_arena_factory_production(slot, selector);
  if (result == ArenaFactorySelection::Changed) {
    storage_.banks[0][index] = slot;
    storage_.banks[1][index] = slot;
  }
  return result;
}
ArenaFactorySelection
ArenaConstructionSimulation::select_factory(std::uint16_t index,
                                            std::uint8_t selector) {
  return select_factory_index(index, selector);
}
std::optional<ArenaEntitySlot>
ArenaConstructionSimulation::factory_unit_template(std::uint16_t index) const {
  auto random = random_;
  return initialize_factory_unit_template(index, random);
}
std::optional<ArenaEntitySlot>
ArenaConstructionSimulation::initialize_factory_unit_template(
    std::uint16_t index, ArenaRandom &random) const {
  const auto &bank = storage_.banks[storage_.read_bank];
  if (index >= bank.size())
    return {};
  const auto &factory = bank[index];
  if ((factory[15] != 26 && factory[15] != 28) || factory[0xaa] > 4)
    return {};
  std::array<float, 4> position{};
  for (unsigned axis = 0; axis < 3; ++axis) {
    std::uint32_t bits = 0;
    for (unsigned i = 0; i < 4; ++i)
      bits = (bits << 8) | factory[4 * axis + i];
    position[axis] = std::bit_cast<float>(bits);
  }
  ArenaEntitySlot unit{};
  const unsigned base = factory[15] == 26 ? 40 : 44;
  if (!initialize_arena_unit_template(unit, base + factory[0xaa],
                                      factory[0x33] >> 5, position, random))
    return {};
  return unit;
}
std::optional<ArenaGroundUnitTemplate>
ArenaConstructionSimulation::factory_ground_unit_template(
    std::uint16_t index) const {
  auto random = random_;
  auto unit = initialize_factory_unit_template(index, random);
  if (!unit)
    return {};
  std::array<ArenaUnitTargetPlayer, 4> players{};
  for (unsigned owner = 0; owner < config_.players.size(); ++owner)
    players[owner] = {
        static_cast<std::uint8_t>(config_.players[owner].route.team),
        config_.unit_base_available[owner],
        config_.unit_target_overrides[owner]};
  const auto binding = bind_arena_ground_unit_template(
      *unit, std::span(players).first(config_.players.size()), grid_, random);
  if (binding == ArenaGroundUnitBinding::Unsupported)
    return {};
  return ArenaGroundUnitTemplate{*unit, binding,
                                 *arena_unit_constructor_result((*unit)[15])};
}
ArenaTimedSubmit ArenaConstructionSimulation::enqueue_event(std::size_t owner,
                                                            ArenaEvent e) {
  if (owner >= players_.size() || !std::isfinite(e.time) ||
      (e.selector < 9 && (e.grid_x >= config_.native_extent || e.grid_z >= config_.native_extent)))
    return ArenaTimedSubmit::Invalid;
  auto &p = players_[owner];
  if (p.events.size() >= p.event_capacity)
    return ArenaTimedSubmit::Full;
  p.events.push(e);
  return ArenaTimedSubmit::Scheduled;
}
bool ArenaConstructionSimulation::queue_ground_health_delta(std::uint16_t index,float delta){
  if(index>=storage_.banks[0].size() || index>=storage_.banks[1].size() || index>=health_deltas_.size())return false;
  const auto& slot=storage_.banks[storage_.read_bank][index];
  const auto kind=slot[15];
  if(!(kind==40 || kind==41 || (kind>=43 && kind<=47)) || (slot[0x33]&28)!=20 ||
     ((unsigned(slot[12])<<8)|slot[13])!=index || storage_.banks[0][index]!=storage_.banks[1][index])return false;
  return accumulate_arena_health_delta(health_deltas_[index],delta);
}
ArenaConstructionTick ArenaConstructionSimulation::advance(float clock) {
  if (!std::isfinite(clock) || clock < clock_)
    throw std::invalid_argument("clock must be finite and monotonic");
  ArenaConstructionTick tick;
  const float elapsed = clock - clock_;
  const bool deploying=std::any_of(config_.ground_deployment.begin(),config_.ground_deployment.end(),[](const auto &c){return c.has_value();});
  if ((config_.ground_transport_enabled || deploying) && elapsed > 10)
    throw std::invalid_argument("transport tick is limited to ten seconds");
  if (elapsed > 0) {
    auto next = storage_.banks[storage_.read_bank];
    for (std::size_t i = 0; i < next.size(); ++i) {
      if (!supported(next[i][15]))
        continue;
      if (advance_arena_structure_construction(next[i], elapsed) ==
          ArenaConstructionProgress::Ready)
        tick.ready_entities.push_back(static_cast<std::uint16_t>(i));
    }
    auto quotas = config_.unit_quotas;
    for (std::size_t i = 0; i < next.size(); ++i) {
      if (next[i][15] != 26 && next[i][15] != 28)
        continue;
      const auto owner = static_cast<std::size_t>(next[i][0x33] >> 5);
      if (owner >= players_.size())
        throw std::invalid_argument("invalid factory owner");
      if (advance_arena_factory_production(
              next[i], elapsed, config_.production_enabled &&
                  (!config_.native_opponents[owner] || native_opponent_active(owner,statistics_)), quotas[owner]) ==
          ArenaFactoryProduction::Reserved)
        tick.production_reservations.push_back(static_cast<std::uint16_t>(i));
    }
    if (config_.ground_transport_enabled || deploying) {
      auto transported = storage_;
      transported.banks[0] = next;
      transported.banks[1] = std::move(next);
      auto next_random = random_;
      auto next_deployment_timers=deployment_timers_;
      auto next_statistics=statistics_;
      auto next_health_deltas=health_deltas_;
      auto next_impulses=impulse_deltas_;
      auto next_tesla_commands=tesla_commands_;
      for(unsigned owner=0;owner<players_.size();++owner)
        store_arena_unit_production_quota(next_statistics[owner],quotas[owner]);
      std::array<ArenaUnitTargetPlayer, 4> targets{};
      std::array<ArenaGridPosition,4> bases{};
      std::array<ArenaGroundBase,4> impact_bases{};
      std::array<std::uint8_t,4> visibility_bits{};
      for (unsigned p = 0; p < players_.size(); ++p) {
        targets[p] = {static_cast<std::uint8_t>(routes_[p].team),
                      config_.unit_base_available[p],
                      config_.unit_target_overrides[p]};
        bases[p]=routes_[p].base;
        impact_bases[p]={routes_[p].base,static_cast<std::uint8_t>(routes_[p].team),
                         config_.base_counters[p].has_value() && config_.unit_base_available[p]};
        visibility_bits[p]=static_cast<std::uint8_t>(routes_[p].team);
      }
      // Native preview opponents restrict both new and rebound ground targets.
      // Read staged counters afresh: an earlier unit can eliminate a base this tick.
      auto opponent_targets=[&](unsigned owner) {
        auto filtered=targets;
        if(owner<players_.size() && config_.native_opponents[owner])
          for(unsigned p=0;p<players_.size();++p)
            filtered[p].base_available=targets[p].base_available && !routes_[p].inactive &&
              routes_[p].team!=routes_[owner].team &&
              (!config_.base_counters[p] || next_statistics[p][88]!=0 || next_statistics[p][89]!=0);
        return filtered;
      };
      if(config_.ground_transport_enabled)for (std::size_t i = 0; i < transported.banks[0].size(); ++i) {
        const auto &unit = transported.banks[0][i];
        if (!(unit[15] == 62 || unit[15] == 40 || unit[15] == 41 ||
              (unit[15] >= 43 && unit[15] <= 47))) continue;
        const auto bits = (std::uint32_t(unit[0x84]) << 24) |
                          (std::uint32_t(unit[0x85]) << 16) |
                          (std::uint32_t(unit[0x86]) << 8) | unit[0x87];
        const float speed = unit[15]==62?1.f:std::bit_cast<float>(bits);
        if (!std::isnormal(speed) || speed <= 0 || speed > 64) continue;
        if(unit[15]!=62){
          auto changed=unit;
          if(std::any_of(next_impulses[i].begin(),next_impulses[i].end(),[](float f){return f!=0;}) &&
             apply_arena_impulse(changed,next_impulses[i])==ArenaImpulseDelta::Invalid)
            throw std::invalid_argument("invalid ground impulse");
          next_impulses[i]={};
          if(apply_arena_health_delta(changed,next_health_deltas[i])==ArenaHealthDelta::Invalid)
            throw std::invalid_argument("invalid ground health delta");
          if(update_arena_entity(transported,static_cast<std::uint16_t>(i),changed)!=ArenaEntityUpdate::Updated)
            throw std::invalid_argument("invalid health target membership");
          // Original accumulator is consumed even when the health flag suppresses it.
          next_health_deltas[i]=0;
        }
        // Bound both time and distance: no step travels over a quarter cell.
        const float step = std::min(.05f, .25f / speed);
        const unsigned count = static_cast<unsigned>(std::ceil(elapsed / step));
        const float delta = elapsed / static_cast<float>(count);
        bool moved = false;
        bool transport_stopped = false;
        for (unsigned n = 0; n < count; ++n) {
          if(transported.banks[0][i][15]==62){
            const auto state=advance_arena_ground_corpse(transported,static_cast<std::uint16_t>(i),delta);
            if(state==ArenaGroundCorpseStep::Expired){
              next_impulses[i]={};tick.retired_entities.push_back(static_cast<std::uint16_t>(i));break;
            }
            if(state!=ArenaGroundCorpseStep::Active)
              throw std::invalid_argument("invalid corpse update");
            continue;
          }
          std::array<float,4> contact_impulse{};
          const auto movement_targets=opponent_targets(transported.banks[0][i][0x33]>>5);
          const auto result = transport_stopped ? ArenaGroundTransport::Blocked : transport_arena_ground_unit(
              transported, static_cast<std::uint16_t>(i), delta,
              std::span(movement_targets.data(), players_.size()), grid_, next_random,
              std::span(bases.data(),players_.size()),
              std::span(visibility_bits.data(),players_.size()),
              config_.ground_dynamics_enabled, config_.ground_contacts_enabled,
              std::span(next_statistics.data(),players_.size()),&contact_impulse);
          if(result==ArenaGroundTransport::Moved)moved=true;
          // Original 18030 calls contact/motion 14060, then activation 158B8,
          // then health-depleted lifecycle 6F40. Preserve that per-unit order.
          // The existing ordered native substeps are not original SPU scheduling.
          if(transported.banks[0][i][15]==45){
            auto changed=transported.banks[0][i];
            const auto context_scalar_18=advance_arena_converter_activation(changed,delta);
            if(!context_scalar_18)throw std::invalid_argument("invalid converter activation");
            scalar(changed,0x18,*context_scalar_18);
            if(update_arena_entity(transported,static_cast<std::uint16_t>(i),changed)!=ArenaEntityUpdate::Updated)
              throw std::invalid_argument("invalid converter membership");
          }
          if(result==ArenaGroundTransport::Moved && transported.banks[0][i][15]==44){
            const auto ready=arena_ground_slam_ready(contact_impulse);
            if(!ready)throw std::invalid_argument("invalid ground slam impulse");
            if(*ready){
              const unsigned source_owner=transported.banks[0][i][0x33]>>5;
              if(source_owner>=players_.size())throw std::invalid_argument("invalid ground slam owner");
              std::uint32_t mask=0;
              for(unsigned p=0;p<players_.size();++p)
                if(routes_[p].team!=routes_[source_owner].team)mask|=1u<<p;
              if(!accumulate_arena_ground_slam(transported,static_cast<std::uint16_t>(i),mask,contact_impulse,next_health_deltas))
                throw std::invalid_argument("invalid ground slam gather");
              tick.ground_slams.push_back(static_cast<std::uint16_t>(i));
            }
          }
          const auto &current=transported.banks[0][i];
          const auto owner=static_cast<unsigned>(current[0x33]>>5);
          if(owner>=players_.size())throw std::invalid_argument("invalid impact owner");
          auto scalar_at=[&](unsigned off){unsigned word=0;for(unsigned b=0;b<4;++b)word=(word<<8)|current[off+b];return std::bit_cast<float>(word);};
          auto deceased=current;
          const auto death=begin_arena_ground_death(deceased,next_statistics[owner]);
          if(death==ArenaGroundDeath::Invalid || death==ArenaGroundDeath::Unsupported)
            throw std::invalid_argument("invalid ground death");
          if(death!=ArenaGroundDeath::Alive){
            tick.depleted_units.push_back(static_cast<std::uint16_t>(i));
            if(death==ArenaGroundDeath::Retire){
              if(!retire_arena_entity(transported,static_cast<std::uint16_t>(i)))throw std::invalid_argument("invalid ground retirement");
              next_impulses[i]={};tick.retired_entities.push_back(static_cast<std::uint16_t>(i));break;
            }
            if(update_arena_entity(transported,static_cast<std::uint16_t>(i),deceased)!=ArenaEntityUpdate::Updated)throw std::invalid_argument("invalid corpse conversion");
            continue;
          }
          const auto base=select_arena_ground_base({scalar_at(0),scalar_at(4),scalar_at(8),0},
              static_cast<std::uint8_t>(routes_[owner].team),scalar_at(0x38),
              std::span(impact_bases).first(players_.size()));
          if(base){
            if(read_arena_unit_production_quota(next_statistics[owner]).active<=0 ||
               register_arena_entity_statistics(next_statistics[owner],current,false,true)!=ArenaEntityRegistration::Registered)
              throw std::invalid_argument("invalid impact registration");
            ArenaPlayerResourceDelta impact;impact.base_counter=-1;impact.attribution=static_cast<std::uint16_t>(owner);impact.base_counter_enabled=true;
            apply_arena_player_resource_delta(next_statistics[*base],impact);
            if(!retire_arena_entity(transported,static_cast<std::uint16_t>(i)))
              throw std::invalid_argument("invalid impact storage membership");
            next_impulses[i]={};tick.retired_entities.push_back(static_cast<std::uint16_t>(i));
            tick.base_impacts.push_back({static_cast<std::uint16_t>(i),*base,static_cast<std::uint8_t>(owner)});
            break;
          }
          if (result != ArenaGroundTransport::Moved) {
            if(current[15]!=45)break;
            // Blocked/no-target motion stops retrying, but the independent
            // converter pulse and lifecycle still receive all elapsed time.
            transport_stopped=true;
          }
        }
        if (moved) tick.transported_units.push_back(static_cast<std::uint16_t>(i));
      }
      if(config_.tesla_enabled && config_.ground_transport_enabled){
        const unsigned count=static_cast<unsigned>(std::ceil(elapsed/.05f));
        const float delta=elapsed/static_cast<float>(count);
        for(unsigned n=0;n<count;++n){
          const auto weapons=stage_arena_teslas(transported,grid_,
              std::span(visibility_bits).first(players_.size()),delta,next_random,next_health_deltas,next_tesla_commands);
          tick.spawned_beams.insert(tick.spawned_beams.end(),weapons.spawned.begin(),weapons.spawned.end());
          tick.retired_entities.insert(tick.retired_entities.end(),weapons.retired.begin(),weapons.retired.end());
        }
      }
      if(config_.cannon_enabled && config_.ground_transport_enabled){
        const unsigned count=static_cast<unsigned>(std::ceil(elapsed/.05f));
        const float delta=elapsed/static_cast<float>(count);
        for(unsigned n=0;n<count;++n){
          const auto weapons=stage_arena_cannons(transported,grid_,routes_,delta,next_random,next_impulses);
          tick.spawned_projectiles.insert(tick.spawned_projectiles.end(),weapons.spawned.begin(),weapons.spawned.end());
          tick.cannon_impacts.insert(tick.cannon_impacts.end(),weapons.impacts.begin(),weapons.impacts.end());
          tick.retired_entities.insert(tick.retired_entities.end(),weapons.retired.begin(),weapons.retired.end());
        }
      }
      // Clear pending impulses for freed or recycled records; only live ground owns them.
      for(unsigned i=0;i<next_impulses.size();++i){
        const auto &record=transported.banks[0][i];
        if(!((record[0x33]&28)==20 && (record[15]==40 || record[15]==41 || (record[15]>=43 && record[15]<=47))))next_impulses[i]={};
      }
      // Newly spawned output records start moving on the next native tick,
      // matching the original read/output-bank boundary rather than backdating
      // their movement over the elapsed interval that produced their order.
      for(unsigned owner=0;owner<players_.size();++owner){
        const auto control=config_.ground_deployment[owner];
        if(!control || !config_.unit_base_available[owner])continue;
        if(config_.native_opponents[owner] && !native_opponent_active(owner,next_statistics))continue;
        const auto gate=advance_arena_ground_deployment(next_deployment_timers[owner],*control,elapsed);
        if(gate==ArenaGroundDeploymentGate::Invalid)throw std::invalid_argument("invalid ground deployment timer");
        if(gate!=ArenaGroundDeploymentGate::Ready)continue;
        const auto position=arena_ground_deployment_position(routes_[owner].base);
        if(!position)throw std::invalid_argument("invalid ground deployment base");
        const auto deployment_targets=opponent_targets(owner);
        const auto spawned=spawn_reserved_arena_ground_unit(transported,next_statistics[owner],control->kind,
            static_cast<std::uint8_t>(owner),*position,std::span(deployment_targets).first(players_.size()),grid_,next_random);
        if(spawned.status==ArenaUnitSpawnStatus::Invalid)throw std::invalid_argument("invalid ground deployment spawn");
        tick.ground_deployments.push_back({static_cast<std::uint8_t>(owner),control->kind,spawned});
        if(spawned.status==ArenaUnitSpawnStatus::Spawned){next_deployment_timers[owner]=0;next_impulses[*spawned.entity_index]={};}
      }
      deployment_timers_=next_deployment_timers;
      storage_ = std::move(transported);
      random_ = next_random;
      statistics_=next_statistics;
      health_deltas_=std::move(next_health_deltas);
      impulse_deltas_=std::move(next_impulses);
      tesla_commands_=std::move(next_tesla_commands);
      for(unsigned owner=0;owner<players_.size();++owner)
        quotas[owner]=read_arena_unit_production_quota(statistics_[owner]);
    } else {
      storage_.banks[0] = next;
      storage_.banks[1] = std::move(next);
    }
    config_.unit_quotas = quotas;
    for (unsigned owner = 0; owner < players_.size(); ++owner)
      store_arena_unit_production_quota(statistics_[owner], quotas[owner]);
  }
  clock_ = clock;
  for (std::size_t owner = 0; owner < players_.size(); ++owner) {
    auto &p = players_[owner];
    auto &status = tick.players[owner];
    if (p.dispatch_blocked)
      continue;
    if (p.command_mode == 11) {
      if (p.events.empty() || !(p.events.next().time < clock))
        continue;
      auto e = p.events.next();
      if (!(e.selector == 0 && supported(e.opcode) &&
            config_.tuning[e.opcode]) &&
          !factory_command(e.selector, e.opcode)) {
        status = ArenaTimedStatus::Unsupported;
        continue;
      }
      auto dispatch = dispatch_arena_event(
          p, clock, random_, [&](std::uint8_t x, std::uint8_t z) {
            return resolve_arena_storage_target(
                storage_, grid_.cells[z * 24 + x].visibility,
                config_.event_exclusion_masks[owner], {x, z});
          });
      if (dispatch == ArenaDispatchResult::Retried) {
        status = ArenaTimedStatus::Retried;
        continue;
      }
      if (dispatch != ArenaDispatchResult::Pending)
        continue;
    }
    auto pending = p.pending;
    if (factory_command(pending.selector, pending.opcode)) {
      const auto result =
          select_factory_index(pending.entity_index, pending.selector);
      if (result == ArenaFactorySelection::Changed ||
          result == ArenaFactorySelection::Unchanged) {
        take_arena_pending_command(p);
        status = ArenaTimedStatus::Submitted;
        tick.factory_selections.push_back(
            static_cast<std::uint16_t>(pending.entity_index));
      } else
        status = ArenaTimedStatus::Unsupported;
      continue;
    }
    if (pending.selector != 0 || !supported(pending.opcode) ||
        !config_.tuning[pending.opcode]) {
      status = ArenaTimedStatus::Unsupported;
      continue;
    }
    auto submitted = enqueue({static_cast<std::uint8_t>(owner), pending.opcode,
                              pending.grid_x, pending.grid_z});
    if (submitted == ArenaConstructionSubmit::Queued) {
      take_arena_pending_command(p);
      status = ArenaTimedStatus::Submitted;
    } else if (submitted == ArenaConstructionSubmit::InvalidRequest)
      status = ArenaTimedStatus::Unsupported;
    else if (p.events.size() >= p.event_capacity)
      status = ArenaTimedStatus::Backpressure;
    else {
      schedule_arena_event(
          p, {clock, pending.grid_x, pending.grid_z, 0, pending.opcode}, clock,
          random_);
      take_arena_pending_command(p);
      status = ArenaTimedStatus::Retried;
    }
  }
  if(elapsed>0)advance_native_opponents();
  tick.construction = step();
  return tick;
}
std::optional<std::int16_t> ArenaConstructionSimulation::base_counter(std::size_t owner) const {
  if(owner>=players_.size() || !config_.base_counters[owner])return {};
  return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>((unsigned(statistics_[owner][88])<<8)|statistics_[owner][89]));
}
bool ArenaConstructionSimulation::set_ground_deployment(std::size_t owner,ArenaGroundDeploymentControl control){
  if(owner>=players_.size() || !(control.kind==40 || control.kind==41 || (control.kind>=43 && control.kind<=47)) ||
     !std::isfinite(control.strength) || control.strength<0 || control.strength>1 || std::fpclassify(control.strength)==FP_SUBNORMAL)return false;
  config_.ground_deployment[owner]=control;return true;
}
bool ArenaConstructionSimulation::set_ground_capture_ready(std::uint16_t index,bool ready){
  if(index>=storage_.banks[0].size() || index>=storage_.banks[1].size() ||
     storage_.banks[0][index]!=storage_.banks[1][index] ||
     storage_.banks[0][index][15]!=45 || (storage_.banks[0][index][0x33]&28)!=20)return false;
  for(auto &bank:storage_.banks)bank[index][0xbb]=ready?1:0;
  return true;
}
ArenaUnitSpawnResult ArenaConstructionSimulation::spawn_reserved_ground_unit(
    std::uint8_t owner, std::uint8_t kind,
    const std::array<float, 4> &position) {
  if (owner >= players_.size() || !std::isfinite(position[0]) ||
      !std::isfinite(position[2]) || position[0] < 0 || position[0] >= config_.native_extent ||
      position[2] < 0 || position[2] >= config_.native_extent)
    return {};
  std::array<ArenaUnitTargetPlayer, 4> targets{};
  for (unsigned p = 0; p < players_.size(); ++p)
    targets[p] = {static_cast<std::uint8_t>(config_.players[p].route.team),
                  config_.unit_base_available[p],
                  config_.unit_target_overrides[p]};
  auto result = spawn_reserved_arena_ground_unit(
      storage_, statistics_[owner], kind, owner, position,
      std::span(targets).first(players_.size()), grid_, random_);
  config_.unit_quotas[owner] =
      read_arena_unit_production_quota(statistics_[owner]);
  return result;
}
} // namespace comet::decomp
