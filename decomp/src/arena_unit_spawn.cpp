#include "comet/arena_unit_spawn.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
float f(std::uint32_t v) { return std::bit_cast<float>(v); }
std::uint32_t word(std::span<const std::uint8_t> s, unsigned o) {
  std::uint32_t v = 0;
  for (unsigned i = 0; i < 4; ++i)
    v = (v << 8) | s[o + i];
  return v;
}
void put(std::span<std::uint8_t> s, unsigned o, std::uint32_t v) {
  for (unsigned i = 0; i < 4; ++i)
    s[o + i] = v >> (24 - 8 * i);
}
struct Rounding {
  int previous = std::fegetround();
  Rounding() {
    if (previous < 0 || std::fesetround(FE_TOWARDZERO) != 0)
      throw std::runtime_error("spawn rounding unavailable");
  }
  ~Rounding() { std::fesetround(previous); }
};
float multiply(float a, float b) {
  volatile float v = a * b;
  return v;
}
float add(float a, float b) {
  volatile float v = a + b;
  return v;
}
float subtract(float a, float b) {
  volatile float v = a - b;
  return v;
}
float ratio(std::uint32_t draw) {
  volatile float value = static_cast<float>(draw);
  const float estimate = multiply(value, f(0x30800000));
  const float residual = std::fma(-estimate, f(0x4e7fffff), value);
  return std::fma(residual, f(0x30800000), estimate);
}
bool ground_kind(unsigned kind) {
  return kind == 40 || kind == 41 || (kind >= 43 && kind <= 47);
}
// 1F8B0 arithmetic for a unit record. The native adapter supplies actual cell
// coordinates and commits once; original job staging/merge is separate.
ArenaPackedCellEntry record(const ArenaEntitySlot &s, ArenaGridPosition cell) {
  Rounding rounding;
  ArenaPackedCellEntry e{s[12], s[13], s[14], s[0x6f], 0, 0, 0, s[0x33]};
  const std::array<float, 3> origin{static_cast<float>(cell.x) - 0.5f, -0.5f,
                                    static_cast<float>(cell.z) - 0.5f};
  for (unsigned axis = 0; axis < 3; ++axis) {
    const float local = subtract(f(word(s, 4 * axis)), origin[axis]);
    e[4 + axis] =
        static_cast<std::uint8_t>(multiply(std::clamp(local, 0.0f, 2.0f), 100));
  }
  return e;
}
} // namespace
std::optional<ArenaGroundUnitTemplate> initialize_arena_ground_unit_spawn(
    std::span<const std::uint8_t, 256> reused_slot, std::uint8_t kind,
    std::uint8_t owner, const std::array<float, 4> &position,
    std::span<const ArenaUnitTargetPlayer> players,
    const ArenaRoutingGrid &grid, ArenaRandom &random) {
  if (!ground_kind(kind) || players.empty() || players.size() > 4 ||
      owner >= players.size())
    return {};
  for (float v : position)
    if (!std::isfinite(v) || v < -32767 || v >= 32767)
      return {};
  auto next_random = random;
  ArenaGroundUnitTemplate unit;
  std::copy(reused_slot.begin(), reused_slot.end(), unit.slot.begin());
  std::array<float, 4> jittered;
  {
    Rounding rounding;
    const float x = ratio(next_random.next()), y = ratio(next_random.next()),
                z = ratio(next_random.next());
    const float dx = subtract(x, 0.5f), dz = subtract(z, 0.5f);
    jittered = {add(position[0], multiply(add(dx, dx), f(0x3dcccccc))),
                add(position[1], std::fma(y, f(0x3c23d710), f(0x3e6b851e))),
                add(position[2], multiply(add(dz, dz), f(0x3dcccccc))),
                add(position[3], 0.0f)};
  }
  if (!initialize_arena_unit_template(unit.slot, kind, owner, jittered,
                                      next_random))
    return {};
  unit.binding =
      bind_arena_ground_unit_template(unit.slot, players, grid, next_random);
  if (unit.binding == ArenaGroundUnitBinding::Unsupported)
    return {};
  unit.constructor_result = *arena_unit_constructor_result(kind);
  random = next_random;
  return unit;
}
ArenaUnitSpawnResult spawn_reserved_arena_ground_unit(
    ArenaEntityStorage &storage, std::span<std::uint8_t, 128> statistics,
    std::uint8_t kind, std::uint8_t owner, const std::array<float, 4> &position,
    std::span<const ArenaUnitTargetPlayer> players,
    const ArenaRoutingGrid &grid, ArenaRandom &random) {
  if (!ground_kind(kind) || owner >= players.size() || players.size() > 4)
    return {};
  if (storage.width == 0 || storage.height == 0 || storage.width > 24 ||
      storage.height > 24 || storage.read_bank > 1)
    throw std::invalid_argument("invalid unit storage");
  if (!storage.free_indices.empty()) {
    const auto index = storage.free_indices.front();
    if (index > 65535 || index >= storage.banks[0].size() ||
        index >= storage.banks[1].size())
      throw std::invalid_argument("invalid unit pool index");
  }
  // Validate the complete constructor before committing a claim or RNG draws.
  auto next_random = random;
  const ArenaEntitySlot zero{};
  auto unit = initialize_arena_ground_unit_spawn(zero, kind, owner, position,
                                                 players, grid, next_random);
  if (!unit)
    return {};
  std::array<std::uint8_t, 128> next_stats;
  std::copy(statistics.begin(), statistics.end(), next_stats.begin());
  auto quota = read_arena_unit_production_quota(next_stats);
  if (!claim_arena_unit_production(quota, kind))
    return {ArenaUnitSpawnStatus::NoOrder, {}, {}};
  store_arena_unit_production_quota(next_stats, quota);
  const auto registered =
      register_arena_entity_statistics(next_stats, unit->slot, true, false);
  auto commit = [&] {
    std::copy(next_stats.begin(), next_stats.end(), statistics.begin());
    random = next_random;
  };
  if (registered != ArenaEntityRegistration::Registered) {
    commit();
    return {ArenaUnitSpawnStatus::CapacityRejected, {}, unit->binding};
  }
  put(unit->slot, 0x60, word(unit->slot, 0x60) | 0x00800000u);
  const ArenaGridPosition cell{static_cast<std::uint8_t>(std::clamp(
                                   std::trunc(f(word(unit->slot, 0))), 0.0f,
                                   static_cast<float>(storage.width - 1))),
                               static_cast<std::uint8_t>(std::clamp(
                                   std::trunc(f(word(unit->slot, 8))), 0.0f,
                                   static_cast<float>(storage.height - 1)))};
  const auto flat = cell.z * 24 + cell.x;
  if (storage.free_indices.empty() || storage.cells[0][flat].size() >= 126 ||
      storage.cells[1][flat].size() >= 126) {
    register_arena_entity_statistics(next_stats, unit->slot, false, true);
    commit();
    return {ArenaUnitSpawnStatus::AllocationFailed, {}, unit->binding};
  }
  const auto index = static_cast<std::uint16_t>(storage.free_indices.front());
  unit->slot[12] = index >> 8;
  unit->slot[13] = index;
  const auto packed = record(unit->slot, cell);
  // Reserve all potentially throwing allocations before any owned-state writes.
  for (auto &bank : storage.cells)
    bank[flat].reserve(bank[flat].size() + 1);
  storage.free_indices.pop_front();
  for (unsigned bank = 0; bank < 2; ++bank) {
    storage.banks[bank][index] = unit->slot;
    storage.cells[bank][flat].push_back(packed);
  }
  ++storage.summary_counts[flat];
  commit();
  return {ArenaUnitSpawnStatus::Spawned, index, unit->binding};
}
} // namespace comet::decomp
