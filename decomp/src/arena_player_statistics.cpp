#include "comet/arena_player_statistics.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace comet::decomp {
namespace {
std::uint32_t read(std::span<const std::uint8_t> s, unsigned off,
                   unsigned size = 4) {
  std::uint32_t v = 0;
  for (unsigned i = 0; i < size; ++i)
    v = (v << 8) | s[off + i];
  return v;
}
void write(std::span<std::uint8_t> s, unsigned off, std::uint32_t v,
           unsigned size = 4) {
  for (unsigned i = 0; i < size; ++i)
    s[off + i] = v >> (8 * (size - 1 - i));
}
float scalar(std::span<const std::uint8_t> s, unsigned off) {
  return std::bit_cast<float>(read(s, off));
}
void write_scalar(std::span<std::uint8_t> s, unsigned off, float v) {
  write(s, off, std::bit_cast<std::uint32_t>(v));
}
std::int16_t signed_half(std::span<const std::uint8_t> s, unsigned off) {
  return std::bit_cast<std::int16_t>(
      static_cast<std::uint16_t>(read(s, off, 2)));
}
float normal(float v) { return std::fpclassify(v) == FP_SUBNORMAL ? 0.0f : v; }
void finite(float v) {
  if (!std::isfinite(v))
    throw std::invalid_argument("unsupported player statistics float");
}
float add(float a, float b) {
  finite(a);
  finite(b);
  a = normal(a);
  b = normal(b);
  // SPU extended exponent values have no supported native IEEE encoding here.
  const double sum = static_cast<double>(a) + static_cast<double>(b);
  if (std::abs(sum) > std::numeric_limits<float>::max())
    throw std::invalid_argument("player statistics float outside native range");
  volatile float result = a + b;
  return normal(result);
}
struct Rounding {
  int previous = std::fegetround();
  Rounding() {
    if (previous < 0 || std::fesetround(FE_TOWARDZERO) != 0)
      throw std::runtime_error("player statistics rounding unavailable");
  }
  ~Rounding() { std::fesetround(previous); }
};
} // namespace
bool apply_arena_player_statistics_delta(
    std::span<std::uint8_t, 128> statistics,
    const ArenaPlayerStatisticsDelta &delta) {
  finite(delta.ground_weight);
  finite(delta.air_weight);
  finite(delta.scalar_116);
  for (float v : delta.scalars_00_to_60)
    finite(v);
  const auto count = static_cast<std::int32_t>(signed_half(statistics, 90)) +
                     static_cast<std::int32_t>(read(statistics, 94, 2)) +
                     delta.active;
  const bool accepted =
      count <= std::bit_cast<std::int32_t>(read(statistics, 124));
  std::array<std::uint8_t, 128> updated;
  std::copy(statistics.begin(), statistics.end(), updated.begin());
  {
    Rounding rounding;
    if (accepted) {
      write(updated, 90, read(statistics, 90, 2) + delta.active, 2);
      write_scalar(updated, 64,
                   add(scalar(statistics, 64), delta.ground_weight));
      write_scalar(updated, 68, add(scalar(statistics, 68), delta.air_weight));
    }
    write(updated, 86, read(statistics, 86, 2) + delta.count_86, 2);
    write(updated, 122,
          std::min<std::uint32_t>(250, read(statistics, 122, 2) +
                                           delta.auxiliary_122),
          2);
    updated[92] = statistics[92] + delta.count_92;
    updated[93] = statistics[93] + delta.count_93;
    for (unsigned i = 0; i < 16; ++i)
      write_scalar(updated, i * 4,
                   add(scalar(statistics, i * 4), delta.scalars_00_to_60[i]));
    write_scalar(updated, 116,
                 std::clamp(add(scalar(statistics, 116), delta.scalar_116),
                            2.0f, 16.0f));
  }
  std::copy(updated.begin(), updated.end(), statistics.begin());
  return accepted;
}
ArenaEntityRegistration
register_arena_entity_statistics(std::span<std::uint8_t, 128> statistics,
                                 std::span<const std::uint8_t, 256> slot,
                                 bool adding, bool count_active,
                                 std::uint8_t auxiliary) {
  const unsigned stage = (slot[0x33] >> 2) & 7;
  if (stage < 3 || stage > 6 || slot[15] == 1 ||
      (read(slot, 0x60) & 0x00080000u))
    return ArenaEntityRegistration::UnsupportedEntity;
  const auto sign = static_cast<std::int16_t>(adding ? 1 : -1);
  const float weight = scalar(slot, 0x50);
  finite(weight);
  ArenaPlayerStatisticsDelta delta;
  {
    Rounding rounding;
    // The original computes both products, including weight*0 for structures.
    const float input = normal(weight);
    volatile float ground =
        input * (stage == 5 ? static_cast<float>(sign) : 0.0f);
    volatile float air = input * (stage == 6 ? static_cast<float>(sign) : 0.0f);
    delta.ground_weight = normal(ground);
    delta.air_weight = normal(air);
  }
  delta.active = count_active && stage >= 5 ? sign : 0;
  delta.count_86 = stage <= 4 ? sign : 0;
  delta.auxiliary_122 = auxiliary;
  return apply_arena_player_statistics_delta(statistics, delta)
             ? ArenaEntityRegistration::Registered
             : ArenaEntityRegistration::CapacityRejected;
}
void apply_arena_player_resource_delta(std::span<std::uint8_t,128> statistics,
                                       const ArenaPlayerResourceDelta &delta) {
  std::array<std::uint8_t,128> next;
  std::copy(statistics.begin(),statistics.end(),next.begin());
  {
    Rounding rounding;
    const float capacity=add(scalar(statistics,76),delta.capacity);
    const float resource=add(scalar(statistics,72),delta.resource);
    write_scalar(next,76,capacity);
    write_scalar(next,72,resource>capacity?capacity:resource);
    write_scalar(next,80,add(scalar(statistics,80),delta.scalar_80));
    write(next,84,std::min<std::uint32_t>(10000,read(statistics,84,2)+delta.count_84),2);
    const auto old=signed_half(statistics,88);
    const auto change=delta.base_counter_enabled?delta.base_counter:0;
    const auto sum=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(read(statistics,88,2)+change));
    write(next,88,static_cast<std::uint16_t>(std::max<std::int16_t>(sum,0)),2);
    if(old>0 && change<0)write(next,120,delta.attribution,2);
  }
  std::copy(next.begin(),next.end(),statistics.begin());
}
ArenaUnitProductionQuota read_arena_unit_production_quota(
    std::span<const std::uint8_t, 128> statistics) {
  ArenaUnitProductionQuota q;
  q.active = signed_half(statistics, 90);
  q.pending = static_cast<std::uint16_t>(read(statistics, 94, 2));
  q.capacity = read(statistics, 124);
  for (unsigned i = 0; i < q.by_type.size(); ++i)
    q.by_type[i] = static_cast<std::uint16_t>(read(statistics, 96 + 2 * i, 2));
  return q;
}
void store_arena_unit_production_quota(std::span<std::uint8_t, 128> statistics,
                                       const ArenaUnitProductionQuota &quota) {
  write(statistics, 90, static_cast<std::uint16_t>(quota.active), 2);
  write(statistics, 94, quota.pending, 2);
  write(statistics, 124, quota.capacity);
  for (unsigned i = 0; i < quota.by_type.size(); ++i)
    write(statistics, 96 + 2 * i, quota.by_type[i], 2);
}
} // namespace comet::decomp
