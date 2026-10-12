#include "comet/arena_unit_entity.hpp"
#include <bit>
#include <cassert>
#include <cfenv>
#include <limits>
using namespace comet::decomp;
std::uint32_t word(const ArenaEntitySlot &s, unsigned off) {
  std::uint32_t v = 0;
  for (unsigned i = 0; i < 4; ++i)
    v = (v << 8) | s[off + i];
  return v;
}
int main() {
  assert(arena_unit_constructor_result(40) == 47);
  assert(arena_unit_constructor_result(48) == 54);
  assert(!arena_unit_constructor_result(39));
  assert(!arena_unit_constructor_result(49));
  ArenaEntitySlot s;
  s.fill(0xa5);
  ArenaRandom rng(1), reference(1);
  assert(initialize_arena_unit_template(s, 40, 2, {3.5f, .23f, 4.5f, 0}, rng));
  assert(s[15] == 40 && s[0x6f] == 2 && s[14] == 10 && s[0x33] == 85);
  assert(word(s, 0x38) == 0x42480000 && word(s, 0x60) == 0x6006);
  assert(word(s, 0x50) == 0x429fffff);
  assert(word(s, 0x80) == 0x409ccccc && word(s, 0x84) == 0x3fcccccc);
  assert(word(s, 0xa8) == 0x3ff74eca && s[0x32] == 12);
  assert(word(s, 0xc8) == 0x00030004 && word(s, 0xcc) == 0x00030004);
  assert(s[12] == 0xa5 && s[0xba] == 0xa5 && s[0xd0] == 0xa5);
  reference.next();
  assert(rng.next() == reference.next());
  // Each kind selects its original model; no guessed unit names or balance.
  for (unsigned kind = 40; kind <= 48; ++kind) {
    ArenaRandom random(941);
    auto rounding = std::fegetround();
    assert(std::fesetround(FE_UPWARD) == 0);
    assert(
        initialize_arena_unit_template(s, kind, 0, {0, 0, 23.5f, 0}, random));
    assert(std::fegetround() == FE_UPWARD);
    assert(std::fesetround(rounding) == 0);
    assert(s[15] == kind && s[0x6f] == kind - 38);
  }
  const auto before = s;
  ArenaRandom invalid(123), unchanged(123);
  assert(!initialize_arena_unit_template(s, 39, 0, {0, 0, 0, 0}, invalid));
  assert(!initialize_arena_unit_template(s, 49, 0, {0, 0, 0, 0}, invalid));
  assert(!initialize_arena_unit_template(s, 40, 4, {0, 0, 0, 0}, invalid));
  assert(!initialize_arena_unit_template(
      s, 40, 0, {std::numeric_limits<float>::infinity(), 0, 0, 0}, invalid));
  assert(s == before && invalid.next() == unchanged.next());
}
