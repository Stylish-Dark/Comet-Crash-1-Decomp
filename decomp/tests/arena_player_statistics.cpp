#include "comet/arena_player_statistics.hpp"
#include "arena_player_statistics_reference.hpp"
#include <bit>
#include <cassert>
#include <cfenv>
#include <limits>
#include <stdexcept>
using namespace comet::decomp;
using Stats = std::array<std::uint8_t, 128>;
using Slot = std::array<std::uint8_t, 256>;
template <std::size_t N>
void put(std::array<std::uint8_t, N> &s, unsigned o, std::uint32_t v,
         unsigned n = 4) {
  for (unsigned i = 0; i < n; ++i)
    s[o + i] = v >> ((n - 1 - i) * 8);
}
template <std::size_t N>
std::uint32_t get(const std::array<std::uint8_t, N> &s, unsigned o,
                  unsigned n = 4) {
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v = (v << 8) | s[o + i];
  return v;
}
template <std::size_t N>
void fp(std::array<std::uint8_t, N> &s, unsigned o, float f) {
  put(s, o, std::bit_cast<std::uint32_t>(f));
}
Stats base() {
  Stats s{};
  fp(s, 64, 3);
  fp(s, 68, 5);
  fp(s, 116, 7);
  put(s, 90, 4, 2);
  put(s, 94, 2, 2);
  put(s, 124, 10);
  return s;
}
Slot entity(unsigned stage) {
  Slot s{};
  s[15] = 40;
  s[0x33] = (stage << 2) | 1;
  fp(s, 0x50, 2.5f);
  return s;
}
void rejection_keeps_other_writes() {
  auto s = base(), expected = s;
  put(s, 124, 5);
  put(expected, 124, 5);
  ArenaPlayerStatisticsDelta d;
  d.active = 1;
  d.ground_weight = 2.5;
  d.air_weight = -1;
  d.count_86 = -1;
  d.count_92 = 6;
  d.count_93 = 7;
  d.auxiliary_122 = 255;
  d.scalar_116 = 18;
  d.scalars_00_to_60[0] = 1;
  fp(expected, 0, 1);
  put(expected, 86, 65535, 2);
  expected[92] = 6;
  expected[93] = 7;
  put(expected, 122, 250, 2);
  fp(expected, 116, 16);
  assert(!apply_arena_player_statistics_delta(s, d));
  assert(s == expected);
}
void equal_capacity_and_wrap() {
  auto s = base();
  put(s, 124, 7);
  put(s, 86, 32767, 2);
  s[92] = 255;
  s[93] = 254;
  put(s, 122, 249, 2);
  ArenaPlayerStatisticsDelta d;
  d.active = 1;
  d.count_86 = 1;
  d.count_92 = 2;
  d.count_93 = 3;
  d.auxiliary_122 = 2;
  d.ground_weight = 2.5f;
  d.air_weight = -2.5f;
  d.scalar_116 = -20;
  assert(apply_arena_player_statistics_delta(s, d));
  assert(get(s, 90, 2) == 5);
  assert(get(s, 86, 2) == 32768);
  assert(s[92] == 1 && s[93] == 1);
  assert(get(s, 122, 2) == 250);
  assert(get(s, 116) == 0x40000000);
  assert(get(s, 64) == 0x40b00000);
  assert(get(s, 68) == 0x40200000);
  s = {};
  put(s, 90, 32767, 2);
  put(s, 124, 40000);
  assert(apply_arena_player_statistics_delta(s, d));
  assert(get(s, 90, 2) == 32768);
  s = {};
  put(s, 124, 0xffffffff);
  assert(!apply_arena_player_statistics_delta(s, {}));
  assert(get(s, 116) == 0x40000000);
}
void stage_registration() {
  for (unsigned stage = 0; stage < 8; ++stage)
    for (bool add : {false, true})
      for (bool count : {false, true}) {
        auto s = base(), expected = s;
        auto slot = entity(stage);
        const bool supported = stage >= 3 && stage <= 6;
        const int sign = add ? 1 : -1;
        if (supported) {
          if (stage <= 4)
            put(expected, 86, static_cast<std::uint16_t>(sign), 2);
          else {
            fp(expected, stage == 5 ? 64 : 68,
               (stage == 5 ? 3.0f : 5.0f) + sign * 2.5f);
            if (count)
              put(expected, 90, 4 + sign, 2);
          }
          put(expected, 122, 3, 2);
        }
        auto result = register_arena_entity_statistics(s, slot, add, count, 3);
        assert(result == (supported
                              ? ArenaEntityRegistration::Registered
                              : ArenaEntityRegistration::UnsupportedEntity));
        assert(s == expected);
      }
  for (unsigned stage = 3; stage <= 6; ++stage)
    for (unsigned blocked = 0; blocked < 2; ++blocked) {
      auto s = base(), before = s;
      auto slot = entity(stage);
      if (blocked)
        put(slot, 0x60, 0x00080000);
      else
        slot[15] = 1;
      assert(register_arena_entity_statistics(s, slot, true, true) ==
             ArenaEntityRegistration::UnsupportedEntity);
      assert(s == before);
    }
}
void claim_register_and_compensate() {
  auto s = base();
  auto q = read_arena_unit_production_quota(s);
  q.by_type[0] = 1;
  store_arena_unit_production_quota(s, q);
  assert(claim_arena_unit_production(q, 40) == 40);
  store_arena_unit_production_quota(s, q);
  assert(register_arena_entity_statistics(s, entity(5), true, false) ==
         ArenaEntityRegistration::Registered);
  auto claimed = read_arena_unit_production_quota(s);
  assert(claimed.active == 5 && claimed.pending == 1 &&
         claimed.by_type[0] == 0);
  assert(get(s, 64) == 0x40b00000);
  assert(register_arena_entity_statistics(s, entity(5), false, true) ==
         ArenaEntityRegistration::Registered);
  auto rolled = read_arena_unit_production_quota(s);
  assert(rolled.active == 4 && rolled.pending == 1 && rolled.by_type[0] == 0);
  assert(get(s, 64) == 0x40400000);
  auto untouched = s;
  q = {};
  q.active = -32768;
  q.pending = 65535;
  q.capacity = 0xffffffff;
  for (unsigned i = 0; i < 9; ++i)
    q.by_type[i] = i * 31;
  store_arena_unit_production_quota(s, q);
  const auto read = read_arena_unit_production_quota(s);
  assert(read.active == q.active && read.pending == q.pending &&
         read.capacity == q.capacity && read.by_type == q.by_type);
  for (unsigned i = 0; i < 128; ++i)
    if (!(i >= 90 && i < 92) && !(i >= 94 && i < 114) && i < 124)
      assert(s[i] == untouched[i]);
}
void numeric_guards_and_rounding() {
  auto s = base();
  ArenaPlayerStatisticsDelta d;
  d.scalars_00_to_60[0] = std::numeric_limits<float>::infinity();
  auto before = s;
  bool caught = false;
  try {
    apply_arena_player_statistics_delta(s, d);
  } catch (const std::invalid_argument &) {
    caught = true;
  }
  assert(caught && s == before);
  d = {};
  d.ground_weight = std::numeric_limits<float>::max();
  fp(s, 64, std::numeric_limits<float>::max());
  before = s;
  caught = false;
  try {
    apply_arena_player_statistics_delta(s, d);
  } catch (const std::invalid_argument &) {
    caught = true;
  }
  assert(caught && s == before);
  s = base();
  put(s, 0, 0x007fffff);
  put(s, 4, 0x807fffff);
  d = {};
  d.scalars_00_to_60[2] = std::bit_cast<float>(0x3f800001u);
  put(s, 8, 0x33800000);
  for (int mode : {FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO}) {
    auto copy = s;
    assert(std::fesetround(mode) == 0);
    assert(apply_arena_player_statistics_delta(copy, d));
    assert(std::fegetround() == mode);
    assert(get(copy, 0) == 0 && get(copy, 4) == 0);
    assert(get(copy, 8) == 0x3f800001u);
  }
  assert(std::fesetround(FE_TONEAREST) == 0);
}
template <std::size_t N> std::array<std::uint8_t, N> decode(const char *hex) {
  std::array<std::uint8_t, N> bytes{};
  auto digit = [](char c) -> unsigned {
    return c <= '9' ? c - '0' : c - 'a' + 10;
  };
  for (unsigned i = 0; i < N; ++i)
    bytes[i] = (digit(hex[2 * i]) << 4) | digit(hex[2 * i + 1]);
  return bytes;
}
void original_numeric_references() {
  for (const auto &r : player_statistics_references) {
    auto s = decode<128>(r.statistics);
    bool accepted;
    if (r.mode) {
      auto slot = entity(r.stage);
      slot[15] = r.type;
      put(slot, 0x60, r.flags);
      put(slot, 0x50, r.weight);
      accepted = register_arena_entity_statistics(
                     s, slot, r.adding, r.count_active, r.auxiliary) ==
                 ArenaEntityRegistration::Registered;
    } else {
      ArenaPlayerStatisticsDelta d;
      const auto vectors = decode<64>(r.vectors);
      const auto h = decode<16>(r.header);
      for (unsigned i = 0; i < 16; ++i)
        d.scalars_00_to_60[i] = std::bit_cast<float>(get(vectors, 4 * i));
      d.ground_weight = std::bit_cast<float>(get(h, 0));
      d.air_weight = std::bit_cast<float>(get(h, 4));
      d.active =
          std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(get(h, 8, 2)));
      d.count_86 = std::bit_cast<std::int16_t>(
          static_cast<std::uint16_t>(get(h, 10, 2)));
      d.count_92 = h[13];
      d.count_93 = h[14];
      d.auxiliary_122 = h[15];
      d.scalar_116 = std::bit_cast<float>(r.timer);
      accepted = apply_arena_player_statistics_delta(s, d);
    }
    assert(accepted == r.accepted);
    assert(s == decode<128>(r.expected));
  }
}
void unsupported_numeric_preserves_all_bytes() {
  for (unsigned field : {0u, 60u, 64u, 68u, 116u})
    for (std::uint32_t value : {0x7f800000u, 0xff800000u, 0x7fc00001u}) {
      auto s = base();
      put(s, field, value);
      const auto before = s;
      bool caught = false;
      try {
        apply_arena_player_statistics_delta(s, {});
      } catch (const std::invalid_argument &) {
        caught = true;
      }
      assert(caught && s == before);
    }
  auto s = base();
  const auto before = s;
  auto slot = entity(5);
  put(slot, 0x50, 0x7fc00000u);
  bool caught = false;
  try {
    register_arena_entity_statistics(s, slot, true, false);
  } catch (const std::invalid_argument &) {
    caught = true;
  }
  assert(caught && s == before);
  // Rejected entities do not inspect numeric fields or touch the environment.
  slot[15] = 1;
  assert(register_arena_entity_statistics(s, slot, true, false) ==
         ArenaEntityRegistration::UnsupportedEntity);
  assert(s == before);
}
int main() {
  rejection_keeps_other_writes();
  equal_capacity_and_wrap();
  stage_registration();
  claim_register_and_compensate();
  numeric_guards_and_rounding();
  original_numeric_references();
  unsupported_numeric_preserves_all_bytes();
}
