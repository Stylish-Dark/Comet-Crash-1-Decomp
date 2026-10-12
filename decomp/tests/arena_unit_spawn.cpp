#include "comet/arena_unit_spawn.hpp"
#include "arena_unit_spawn_reference.hpp"
#include "comet/native_construction_scene.hpp"
#include <bit>
#include <cassert>
#include <cfenv>
#include <limits>
using namespace comet::decomp;
using Stats = std::array<std::uint8_t, 128>;
float scalar(std::span<const std::uint8_t> s, unsigned o) {
  unsigned v = 0;
  for (unsigned i = 0; i < 4; ++i)
    v = (v << 8) | s[o + i];
  return std::bit_cast<float>(v);
}
ArenaEntityStorage storage(unsigned count) {
  ArenaEntityStorage s;
  for (auto &b : s.banks)
    b.resize(count);
  reset_arena_entity_index_pool(s, count);
  return s;
}
Stats statistics(unsigned count) {
  Stats s{};
  ArenaUnitProductionQuota q;
  q.capacity = 20;
  q.pending = count;
  q.by_type[0] = count;
  store_arena_unit_production_quota(s, q);
  return s;
}
void success_and_failure() {
  ArenaRoutingGrid grid;
  std::array<ArenaUnitTargetPlayer, 2> players{
      {{0, true, 255}, {1, true, 255}}};
  auto st = storage(1);
  auto stats = statistics(2);
  ArenaRandom r(1);
  auto result = spawn_reserved_arena_ground_unit(
      st, stats, 40, 0, {2.5f, 0, 3.5f, 0}, players, grid, r);
  assert(result.status == ArenaUnitSpawnStatus::Spawned &&
         result.entity_index == 0 &&
         result.binding == ArenaGroundUnitBinding::Bound);
  const auto q = read_arena_unit_production_quota(stats);
  assert(q.active == 1 && q.pending == 1 && q.by_type[0] == 1);
  assert(st.banks[0] == st.banks[1] && st.banks[0][0][15] == 40 &&
         st.banks[0][0][0x6f] == 2);
  assert(st.free_indices.empty() && st.summary_counts[3 * 24 + 2] == 1);
  for (const auto &cells : st.cells) {
    assert(cells[3 * 24 + 2].size() == 1);
    const auto &e = cells[3 * 24 + 2][0];
    assert(e[3] == 2 && e[7] == st.banks[0][0][0x33] && e[0] == 0 && e[1] == 0);
  }
  assert((st.banks[0][0][0x61] & 0x80) != 0);
  const float weight = scalar(st.banks[0][0], 0x50);
  assert(scalar(stats, 64) == weight);
  const auto bank = st.banks;
  const auto lists = st.cells;
  result = spawn_reserved_arena_ground_unit(
      st, stats, 40, 0, {2.5f, 0, 3.5f, 0}, players, grid, r);
  assert(result.status == ArenaUnitSpawnStatus::AllocationFailed &&
         !result.entity_index);
  const auto after = read_arena_unit_production_quota(stats);
  assert(after.active == 1 && after.pending == 0 && after.by_type[0] == 0);
  assert(st.banks == bank && st.cells == lists && scalar(stats, 64) == weight);
  auto untouched = stats;
  auto expected_rng = r;
  result = spawn_reserved_arena_ground_unit(
      st, stats, 40, 0, {2.5f, 0, 3.5f, 0}, players, grid, r);
  assert(result.status == ArenaUnitSpawnStatus::NoOrder && stats == untouched &&
         r.next() == expected_rng.next());
}
void invalid_and_crossing() {
  ArenaRoutingGrid grid;
  std::array<ArenaUnitTargetPlayer, 1> players{{{0, true, 255}}};
  for (unsigned kind : {39u, 42u, 48u, 49u}) {
    auto st = storage(2);
    auto stats = statistics(1), before = stats;
    ArenaRandom r(4), copy = r;
    assert(spawn_reserved_arena_ground_unit(st, stats, kind, 0, {1, 0, 1, 0},
                                            players, grid, r)
               .status == ArenaUnitSpawnStatus::Invalid);
    assert(stats == before && r.next() == copy.next() &&
           st.free_indices.size() == 2);
  }
  ArenaEntitySlot reused{};
  reused.fill(0xa5);
  for (unsigned seed = 1; seed < 100; ++seed) {
    auto st = storage(1);
    auto stats = statistics(1);
    ArenaRandom r(seed);
    auto result = spawn_reserved_arena_ground_unit(
        st, stats, 40, 0, {2, 0, 3, 0}, players, grid, r);
    assert(result.status == ArenaUnitSpawnStatus::Spawned);
    const auto &slot = st.banks[0][0];
    const unsigned x = static_cast<unsigned>(scalar(slot, 0)),
                   z = static_cast<unsigned>(scalar(slot, 8));
    assert(st.cells[0][z * 24 + x].size() == 1 &&
           st.summary_counts[z * 24 + x] == 1);
  }
  for (int mode : {FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO}) {
    assert(std::fesetround(mode) == 0);
    ArenaRandom r(1);
    auto unit = initialize_arena_ground_unit_spawn(
        reused, 40, 0, {2.5f, 0, 3.5f, 0}, players, grid, r);
    assert(unit && std::fegetround() == mode && unit->slot[0xf0] == 0xa5);
  }
  assert(std::fesetround(FE_TONEAREST) == 0);
  auto st = storage(1);
  auto stats = statistics(1), before = stats;
  ArenaRandom r(1), copy = r;
  assert(spawn_reserved_arena_ground_unit(
             st, stats, 40, 0,
             {std::numeric_limits<float>::quiet_NaN(), 0, 3, 0}, players, grid,
             r)
             .status == ArenaUnitSpawnStatus::Invalid);
  assert(stats == before && r.next() == copy.next());
}
void owned_scene_integration() {
  ArenaConstructionSimulationConfig config;
  config.capacity = 4;
  config.players = {{{{0, 0}, 0, false}, 400, 400}};
  config.tuning[26] = ArenaStructureTableValues{100, 50, 10, 20, 7.5f};
  config.unit_quotas[0].capacity = 2;
  config.production_enabled = true;
  ArenaState arena;
  arena.extent = 8;
  arena.players.resize(1);
  arena.players[0].start_grid = ArenaGridPosition{0, 0};
  NativeConstructionScene scene(arena, config);
  assert(scene.enqueue({0, 26, 2, 2}) == ArenaConstructionSubmit::Queued);
  scene.step();
  scene.advance(2);
  scene.advance(4);
  assert(scene.simulation().unit_quotas()[0].pending == 1);
  auto result = scene.spawn_reserved_ground_unit(0, 40, {2, 2});
  assert(result.status == ArenaUnitSpawnStatus::Spawned &&
         result.entity_index == 1);
  assert(scene.arena().entities.size() == 2 &&
         scene.arena().cells[50].entity_index == 0);
  const auto &unit = scene.arena().entities[1];
  assert(unit.type == 40 && unit.model_slot == 2 &&
         unit.live_entity_index == 1 && !unit.construction_progress &&
         !unit.production_mode);
  assert(scene.simulation().unit_quotas()[0].active == 1 &&
         scene.simulation().unit_quotas()[0].pending == 0 &&
         scene.simulation().accounts()[0].resource == 340);
  const auto bytes = scene.simulation().storage().banks[0][1];
  scene.advance(5);
  assert(scene.simulation().storage().banks[0][1] == bytes &&
         !scene.arena().entities[1].construction_progress);
  assert(scene.select_factory(0, {2, 2}, 8) == ArenaFactorySelection::Changed);
  assert(scene.spawn_reserved_ground_unit(0, 40, {8, 0}).status ==
         ArenaUnitSpawnStatus::Invalid);
}
template <std::size_t N> std::array<std::uint8_t, N> decode(const char *hex) {
  std::array<std::uint8_t, N> v{};
  auto digit = [](char c) -> unsigned {
    return c <= '9' ? c - '0' : c - 'a' + 10;
  };
  for (unsigned i = 0; i < N; ++i)
    v[i] = (digit(hex[2 * i]) << 4) | digit(hex[2 * i + 1]);
  return v;
}
void original_spawn_references() {
  ArenaRoutingGrid grid;
  for (unsigned cell = 0; cell < 576; ++cell)
    for (unsigned pair = 0; pair < 12; ++pair)
      grid.cells[cell].routes[pair] = (17 * cell + 53 * pair) & 255;
  const std::array<ArenaUnitTargetPlayer, 2> players{
      {{0, true, 255}, {1, true, 255}}};
  for (const auto &ref : ground_spawn_references) {
    auto st = storage(4);
    st.free_indices.clear();
    if (ref.available)
      st.free_indices.push_back(3);
    if (ref.staging_full)
      for (auto &bank : st.cells)
        for (auto &c : bank)
          c.resize(126);
    Stats stats{};
    ArenaUnitProductionQuota q;
    q.pending = 1;
    q.by_type[ref.kind - 40] = 1;
    q.capacity = 1;
    store_arena_unit_production_quota(stats, q);
    ArenaRandom random(ref.kind);
    auto result = spawn_reserved_arena_ground_unit(
        st, stats, ref.kind, 0, {2.5f, 0, 3.5f, 0}, players, grid, random);
    assert((result.status == ArenaUnitSpawnStatus::Spawned) == ref.spawned);
    assert(stats == decode<128>(ref.statistics));
    assert(st.banks[0][3] == decode<256>(ref.slot) &&
           st.banks[1][3] == decode<256>(ref.slot));
    if (ref.spawned)
      assert(st.cells[0][3 * 24 + 2].back() == decode<8>(ref.record));
    assert(random.next() == ref.next_random);
  }
}
void outer_boundary_and_capacity_rejection() {
  ArenaRoutingGrid grid;
  const std::array<ArenaUnitTargetPlayer, 1> players{{{0, true, 255}}};
  bool crossed = false, stayed = false, negative = false;
  for (unsigned seed = 1; seed < 100; ++seed) {
    auto st = storage(1);
    auto stats = statistics(1), before = stats;
    ArenaRandom r(seed), copy = r;
    const auto result = spawn_reserved_arena_ground_unit(
        st, stats, 40, 0, {23.99f, 0, 23.99f, 0}, players, grid, r);
    if (result.status == ArenaUnitSpawnStatus::Invalid) {
      crossed = true;
      assert(stats == before && st.free_indices.size() == 1 &&
             r.next() == copy.next());
    } else {
      stayed = true;
      assert(result.status == ArenaUnitSpawnStatus::Spawned &&
             st.cells[0][23 * 24 + 23].size() == 1);
    }
    auto low = storage(1);
    stats = statistics(1);
    ArenaRandom low_rng(seed);
    assert(spawn_reserved_arena_ground_unit(
               low, stats, 40, 0, {0.01f, 0, 0.01f, 0}, players, grid, low_rng)
               .status == ArenaUnitSpawnStatus::Spawned);
    negative |= scalar(low.banks[0][0], 0) < 0;
    assert(low.cells[0][0].size() == 1);
  }
  assert(crossed && stayed && negative);
  auto st = storage(1);
  auto stats = statistics(1);
  auto q = read_arena_unit_production_quota(stats);
  q.capacity = 0;
  store_arena_unit_production_quota(stats, q);
  ArenaRandom r(1), expected = r;
  for (unsigned i = 0; i < 4; ++i)
    expected.next();
  const auto result = spawn_reserved_arena_ground_unit(
      st, stats, 40, 0, {2.5f, 0, 3.5f, 0}, players, grid, r);
  assert(result.status == ArenaUnitSpawnStatus::CapacityRejected &&
         !result.entity_index && st.free_indices.size() == 1);
  q = read_arena_unit_production_quota(stats);
  assert(q.active == 1 && q.pending == 0 && q.by_type[0] == 0 &&
         scalar(stats, 64) == 0 && r.next() == expected.next());
}
int main() {
  success_and_failure();
  invalid_and_crossing();
  owned_scene_integration();
  original_spawn_references();
  outer_boundary_and_capacity_rejection();
}
