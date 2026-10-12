#include "comet/arena_factory_production.hpp"
#include <bit>
#include <cassert>
#include <cfenv>
#include <limits>
#include <stdexcept>
using namespace comet::decomp;
void put(ArenaEntitySlot &s, unsigned off, std::uint32_t word) {
  for (unsigned i = 0; i < 4; ++i)
    s[off + i] = word >> (24 - i * 8);
}
float scalar(const ArenaEntitySlot &s, unsigned off) {
  std::uint32_t word = 0;
  for (unsigned i = 0; i < 4; ++i)
    word = (word << 8) | s[off + i];
  return std::bit_cast<float>(word);
}
int main() {
  ArenaEntitySlot slot{};
  slot[15] = 26;
  slot[0xaa] = 0;
  put(slot, 0xb8, 40);
  put(slot, 0x60, 0x01000000);
  ArenaUnitProductionQuota quota;
  quota.capacity = 2;
  assert(advance_arena_factory_production(slot, 3, true, quota) ==
         ArenaFactoryProduction::Progress);
  assert(quota.pending == 0 && scalar(slot, 0x84) == 3.5f);
  assert(advance_arena_factory_production(slot, .25f, true, quota) ==
         ArenaFactoryProduction::Reserved);
  assert(quota.pending == 1 && quota.by_type[0] == 1);
  assert(scalar(slot, 0x80) == 3.25f);
  assert(advance_arena_factory_production(slot, 1, true, quota) ==
         ArenaFactoryProduction::Progress);
  assert(scalar(slot, 0x80) == 0 && quota.pending == 1);
  assert(advance_arena_factory_production(slot, 3.3f, true, quota) ==
         ArenaFactoryProduction::Reserved);
  assert(quota.pending == 2);
  advance_arena_factory_production(slot, 1, true, quota);
  assert(advance_arena_factory_production(slot, 3.3f, true, quota) ==
         ArenaFactoryProduction::Full);
  assert(quota.pending == 2);
  assert(advance_arena_factory_production(slot, 1, false, quota) ==
         ArenaFactoryProduction::Blocked);
  assert(scalar(slot, 0x80) == 0);
  put(slot, 0x80, std::bit_cast<std::uint32_t>(3.3f));
  quota.capacity = 2;
  assert(advance_arena_factory_production(slot, 1, true, quota) ==
         ArenaFactoryProduction::Progress);
  assert(scalar(slot, 0x80) == 0);
  assert(advance_arena_factory_production(slot, 3.3f, true, quota) ==
         ArenaFactoryProduction::Full);
  const auto pinned = scalar(slot, 0x80);
  assert(pinned > 3.19f && pinned < 3.21f);
  quota.capacity = 3;
  assert(advance_arena_factory_production(slot, .01f, true, quota) ==
         ArenaFactoryProduction::Reserved);
  assert(quota.pending == 3);
  // One reservation per update, even if several periods elapsed.
  ArenaUnitProductionQuota fresh;
  fresh.capacity = 10;
  put(slot, 0x80, 0);
  assert(advance_arena_factory_production(slot, 100, true, fresh) ==
         ArenaFactoryProduction::Reserved);
  assert(fresh.pending == 1 && scalar(slot, 0x80) == 0);
  slot[15] = 28;
  slot[0xaa] = 4;
  put(slot, 0xb8, 44);
  const auto rounding = std::fegetround();
  assert(std::fesetround(FE_UPWARD) == 0);
  assert(advance_arena_factory_production(slot, 30, true, fresh) ==
         ArenaFactoryProduction::Reserved);
  assert(std::fegetround() == FE_UPWARD);
  assert(std::fesetround(rounding) == 0);
  assert(fresh.by_type[8] == 1 && scalar(slot, 0x84) == 30);
  put(slot, 0x60, 0);
  assert(advance_arena_factory_production(slot, 1, true, fresh) ==
         ArenaFactoryProduction::Blocked);
  assert(scalar(slot, 0x80) == 0 && fresh.pending == 2);
  slot[0xaa] = 5;
  auto unsupported = slot;
  assert(advance_arena_factory_production(slot, 1, true, fresh) ==
         ArenaFactoryProduction::Unsupported);
  assert(slot == unsupported && fresh.pending == 2);
  slot[0xaa] = 4;
  put(slot, 0x80, std::bit_cast<std::uint32_t>(-1.0f));
  unsupported = slot;
  assert(advance_arena_factory_production(slot, 1, true, fresh) ==
         ArenaFactoryProduction::Unsupported);
  assert(slot == unsupported && fresh.pending == 2);
  const auto before = slot;
  for (float invalid : {-1.0f, std::numeric_limits<float>::infinity(),
                        std::numeric_limits<float>::quiet_NaN()}) {
    try {
      advance_arena_factory_production(slot, invalid, true, fresh);
      assert(false);
    } catch (const std::invalid_argument &) {
    }
    assert(slot == before && fresh.pending == 2);
  }
}
