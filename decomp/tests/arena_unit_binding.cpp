#include "comet/arena_unit_binding.hpp"
#include "arena_unit_binding_reference.hpp"
#include <bit>
#include <cassert>
#include <cfenv>
using namespace comet::decomp;
std::uint32_t word(const ArenaEntitySlot &s, unsigned off) {
  std::uint32_t v = 0;
  for (unsigned i = 0; i < 4; ++i)
    v = (v << 8) | s[off + i];
  return v;
}
int main() {
  ArenaRoutingGrid grid;
  std::array<ArenaUnitTargetPlayer, 2> players{
      {{0, true, 255}, {1, true, 255}}};
  ArenaEntitySlot unit{};
  ArenaRandom random(1), reference(1);
  assert(initialize_arena_unit_template(unit, 40, 0, {3.5f, .23f, 4.5f, 0},
                                        random));
  reference.next();
  assert(bind_arena_ground_unit_template(unit, players, grid, random) ==
         ArenaGroundUnitBinding::Bound);
  reference.next();
  assert(random.next() == reference.next());
  assert(unit[0x7e] == 1 && word(unit, 0x24) == 0xbf6c835e &&
         word(unit, 0x2c) == 0x3ec3ef16);
  assert(word(unit, 0x40) == 0x3f90d0c1 && word(unit, 0x48) == 0x3f90d0c1);
  assert(word(unit, 0x4c) == 0);
  // Blocked route preserves orientation and velocity but still chooses target.
  ArenaRandom blocked(3);
  initialize_arena_unit_template(unit, 40, 0, {3.5f, 0, 4.5f, 0}, blocked);
  const auto before = unit;
  grid.cells[4 * 24 + 3].routes[0] = 1;
  assert(bind_arena_ground_unit_template(unit, players, grid, blocked) ==
         ArenaGroundUnitBinding::Blocked);
  for (unsigned i = 0; i < 256; ++i)
    assert(unit[i] == (i == 0x7e ? 1 : before[i]));
  // No enemy base means no target-selection RNG draw.
  players[1].base_available = false;
  ArenaRandom empty(9), empty_reference(9);
  initialize_arena_unit_template(unit, 40, 0, {3.5f, 0, 4.5f, 0}, empty);
  empty_reference.next();
  assert(bind_arena_ground_unit_template(unit, players, grid, empty) ==
         ArenaGroundUnitBinding::NoTarget);
  assert(unit[0x7e] == 255 && empty.next() == empty_reference.next());
  // Override bypasses team filtering and the target-selection RNG draw.
  players[0].override_target = 1;
  players[1].base_available = true;
  players[1].team = 0;
  grid.cells[4 * 24 + 3].routes[0] = 10;
  ArenaRandom override_rng(10), override_reference(10);
  initialize_arena_unit_template(unit, 40, 0, {3.5f, 0, 4.5f, 0}, override_rng);
  override_reference.next();
  assert(bind_arena_ground_unit_template(unit, players, grid, override_rng) ==
         ArenaGroundUnitBinding::Bound);
  assert(word(unit, 0x48) == 0xbfcccccc && word(unit, 0x24) == 0x80000000);
  assert(override_rng.next() == override_reference.next());
  // Owner 3 -> target 0 must use directed pair 9, at cached cell (3,4).
  std::array<ArenaUnitTargetPlayer, 4> directed{};
  directed[0].base_available = true;
  directed[3].override_target = 0;
  grid.cells[4 * 24 + 3].routes[9] = 14;
  ArenaRandom directional(13);
  initialize_arena_unit_template(unit, 40, 3, {3.5f, 0, 4.5f, 0}, directional);
  const auto rounding = std::fegetround();
  assert(std::fesetround(FE_UPWARD) == 0);
  assert(bind_arena_ground_unit_template(unit, directed, grid, directional) ==
         ArenaGroundUnitBinding::Bound);
  assert(std::fegetround() == FE_UPWARD);
  assert(std::fesetround(rounding) == 0);
  assert(unit[0x7e] == 0 && word(unit, 0x40) == 0x3fcccccc &&
         word(unit, 0x48) == 0);
  directed[3].override_target = 3;
  const auto invalid_slot = unit;
  auto preserved_rng = directional;
  assert(bind_arena_ground_unit_template(unit, directed, grid, directional) ==
         ArenaGroundUnitBinding::Unsupported);
  assert(unit == invalid_slot && directional.next() == preserved_rng.next());
  // Full-constructor original reference vectors cover every ground
  // kind/heading.
  players = {{{0, true, 255}, {1, true, 255}}};
  for (unsigned k = 0; k < ground_reference::kinds.size(); ++k)
    for (unsigned direction = 0; direction < 8; ++direction) {
      ArenaRandom original_seed(1);
      initialize_arena_unit_template(unit, ground_reference::kinds[k], 0,
                                     {3.5f, .23f, 4.5f, 0}, original_seed);
      grid.cells[4 * 24 + 3].routes[0] =
          static_cast<std::uint8_t>(direction * 2 | 0xf0);
      assert(
          bind_arena_ground_unit_template(unit, players, grid, original_seed) ==
          ArenaGroundUnitBinding::Bound);
      for (unsigned axis = 0; axis < 4; ++axis)
        assert(word(unit, 0x20 + 4 * axis) ==
               ground_reference::headings[direction][axis]);
      assert(word(unit, 0x40) == ground_reference::velocities[k][direction][0]);
      assert(word(unit, 0x48) == ground_reference::velocities[k][direction][1]);
    }
  // Missing override base falls back to the ordered enemy candidate list.
  std::array<ArenaUnitTargetPlayer, 3> fallback{
      {{0, true, 1}, {1, false, 255}, {1, true, 255}}};
  ArenaRandom fallback_rng(21), fallback_reference(21);
  initialize_arena_unit_template(unit, 40, 0, {3.5f, 0, 4.5f, 0}, fallback_rng);
  fallback_reference.next();
  fallback_reference.next();
  assert(bind_arena_ground_unit_template(unit, fallback, grid, fallback_rng) ==
         ArenaGroundUnitBinding::Bound);
  assert(unit[0x7e] == 2 && fallback_rng.next() == fallback_reference.next());
  // Malformed cached coordinates and speed preserve bytes and the RNG.
  const auto valid = unit;
  for (unsigned malformed = 0; malformed < 3; ++malformed) {
    unit = valid;
    if (malformed == 0)
      unit[0xc8] = 1;
    if (malformed == 1) {
      unit[0x84] = 0x7f;
      unit[0x85] = 0x80;
      unit[0x86] = unit[0x87] = 0;
    }
    if (malformed == 2)
      unit[0x33] = static_cast<std::uint8_t>(4 * 32 + 20);
    const auto old = unit;
    auto untouched = fallback_rng;
    assert(
        bind_arena_ground_unit_template(unit, fallback, grid, fallback_rng) ==
        ArenaGroundUnitBinding::Unsupported);
    assert(unit == old && fallback_rng.next() == untouched.next());
  }
  // Aircraft stay outside the ground-direction boundary.
  ArenaRandom invalid(11), invalid_reference(11);
  initialize_arena_unit_template(unit, 42, 0, {3.5f, 0, 4.5f, 0}, invalid);
  invalid_reference.next();
  const auto aircraft = unit;
  assert(bind_arena_ground_unit_template(unit, players, grid, invalid) ==
         ArenaGroundUnitBinding::Unsupported);
  assert(unit == aircraft && invalid.next() == invalid_reference.next());
}
