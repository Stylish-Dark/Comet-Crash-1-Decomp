#include "comet/arena_construction_simulation.hpp"
#include "comet/arena_ground_base.hpp"
#include "comet/arena_ground_lifecycle.hpp"
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
  if(config_.ground_contacts_enabled)config_.ground_dynamics_enabled=true;
  if (c.players.empty() || c.players.size() > 4 || c.capacity > 65536 || !c.native_extent || c.native_extent > 24)
    throw std::invalid_argument("invalid simulation dimensions");
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
ArenaConstructionTick ArenaConstructionSimulation::advance(float clock) {
  if (!std::isfinite(clock) || clock < clock_)
    throw std::invalid_argument("clock must be finite and monotonic");
  ArenaConstructionTick tick;
  const float elapsed = clock - clock_;
  if (config_.ground_transport_enabled && elapsed > 10)
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
              next[i], elapsed, config_.production_enabled, quotas[owner]) ==
          ArenaFactoryProduction::Reserved)
        tick.production_reservations.push_back(static_cast<std::uint16_t>(i));
    }
    if (config_.ground_transport_enabled) {
      auto transported = storage_;
      transported.banks[0] = next;
      transported.banks[1] = std::move(next);
      auto next_random = random_;
      auto next_statistics=statistics_;
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
      for (std::size_t i = 0; i < transported.banks[0].size(); ++i) {
        const auto &unit = transported.banks[0][i];
        if (!(unit[15] == 62 || unit[15] == 40 || unit[15] == 41 ||
              (unit[15] >= 43 && unit[15] <= 47))) continue;
        const auto bits = (std::uint32_t(unit[0x84]) << 24) |
                          (std::uint32_t(unit[0x85]) << 16) |
                          (std::uint32_t(unit[0x86]) << 8) | unit[0x87];
        const float speed = unit[15]==62?1.f:std::bit_cast<float>(bits);
        if (!std::isnormal(speed) || speed <= 0 || speed > 64) continue;
        // Bound both time and distance: no step travels over a quarter cell.
        const float step = std::min(.05f, .25f / speed);
        const unsigned count = static_cast<unsigned>(std::ceil(elapsed / step));
        const float delta = elapsed / static_cast<float>(count);
        bool moved = false;
        for (unsigned n = 0; n < count; ++n) {
          if(transported.banks[0][i][15]==62){
            const auto state=advance_arena_ground_corpse(transported,static_cast<std::uint16_t>(i),delta);
            if(state==ArenaGroundCorpseStep::Expired){
              tick.retired_entities.push_back(static_cast<std::uint16_t>(i));break;
            }
            if(state!=ArenaGroundCorpseStep::Active)
              throw std::invalid_argument("invalid corpse update");
            continue;
          }
          const auto result = transport_arena_ground_unit(
              transported, static_cast<std::uint16_t>(i), delta,
              std::span(targets.data(), players_.size()), grid_, next_random,
              std::span(bases.data(),players_.size()),
              std::span(visibility_bits.data(),players_.size()),
              config_.ground_dynamics_enabled, config_.ground_contacts_enabled,
              std::span(next_statistics.data(),players_.size()));
          if(result==ArenaGroundTransport::Moved)moved=true;
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
              tick.retired_entities.push_back(static_cast<std::uint16_t>(i));break;
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
            tick.retired_entities.push_back(static_cast<std::uint16_t>(i));
            tick.base_impacts.push_back({static_cast<std::uint16_t>(i),*base,static_cast<std::uint8_t>(owner)});
            break;
          }
          if (result != ArenaGroundTransport::Moved) break;
        }
        if (moved) tick.transported_units.push_back(static_cast<std::uint16_t>(i));
      }
      storage_ = std::move(transported);
      random_ = next_random;
      statistics_=next_statistics;
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
  tick.construction = step();
  return tick;
}
std::optional<std::int16_t> ArenaConstructionSimulation::base_counter(std::size_t owner) const {
  if(owner>=players_.size() || !config_.base_counters[owner])return {};
  return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>((unsigned(statistics_[owner][88])<<8)|statistics_[owner][89]));
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
