#include "comet/arena_factory_production.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
std::uint32_t word(std::span<const std::uint8_t, 256> s, unsigned off) {
  std::uint32_t v = 0;
  for (unsigned i = 0; i < 4; ++i)
    v = (v << 8) | s[off + i];
  return v;
}
void put(std::span<std::uint8_t, 256> s, unsigned off, float f) {
  const auto v = std::bit_cast<std::uint32_t>(f);
  for (unsigned i = 0; i < 4; ++i)
    s[off + i] = v >> (24 - i * 8);
}
float normal(float v) { return std::fpclassify(v) == FP_SUBNORMAL ? 0.0f : v; }
float add(float a, float b) {
  volatile float v = normal(a) + normal(b);
  return normal(v);
}
float subtract(float a, float b) {
  volatile float v = normal(a) - normal(b);
  return normal(v);
}
struct Rounding {
  int previous = std::fegetround();
  Rounding() {
    if (previous < 0 || std::fesetround(FE_TOWARDZERO) != 0)
      throw std::runtime_error("production rounding unavailable");
  }
  ~Rounding() { std::fesetround(previous); }
};
} // namespace
std::optional<std::uint8_t>
claim_arena_unit_production(ArenaUnitProductionQuota &quota,
                            std::uint8_t kind) {
  if (kind < 40 || kind > 48 || quota.by_type[kind - 40] == 0)
    return {};
  --quota.by_type[kind - 40];
  --quota.pending;
  quota.active = std::bit_cast<std::int16_t>(
      static_cast<std::uint16_t>(static_cast<std::uint16_t>(quota.active) + 1));
  return kind;
}
ArenaFactoryProduction
advance_arena_factory_production(std::span<std::uint8_t, 256> slot,
                                 float elapsed, bool enabled,
                                 ArenaUnitProductionQuota &quota) {
  if (!std::isfinite(elapsed) || elapsed < 0)
    throw std::invalid_argument("invalid production elapsed time");
  const auto base = word(slot, 0xb8);
  if ((slot[15] != 26 && slot[15] != 28) || slot[0xaa] > 4 ||
      base != (slot[15] == 26 ? 40u : 44u))
    return ArenaFactoryProduction::Unsupported;
  const unsigned kind = base + slot[0xaa] - 40;
  const float old = std::bit_cast<float>(word(slot, 0x80));
  if (!std::isfinite(old) || old < 0 || quota.capacity > 65535)
    return ArenaFactoryProduction::Unsupported;
  constexpr std::array<float, 9> durations{3.5f, 4.5f, 7,  7, 20,
                                           25,   20,   30, 30};
  const float duration = durations[kind];
  auto result = ArenaFactoryProduction::Blocked;
  float timer = 0;
  bool reserved = false;
  {
    Rounding rounding;
    const float threshold = add(
        subtract(subtract(duration, 1.0f), std::bit_cast<float>(0x3e999999u)),
        1.0f);
    if (enabled && (word(slot, 0x60) & 0x01000000u)) {
      timer = add(old, elapsed);
      if (!std::isfinite(timer))
        return ArenaFactoryProduction::Unsupported;
      result = ArenaFactoryProduction::Progress;
      if (old <= threshold && timer > threshold) {
        const auto count =
            static_cast<std::int32_t>(quota.active) + 1 + quota.pending;
        if (count <= static_cast<std::int32_t>(quota.capacity)) {
          reserved = true;
          result = ArenaFactoryProduction::Reserved;
        } else {
          timer = threshold;
          result = ArenaFactoryProduction::Full;
        }
      }
      if (timer > duration)
        timer = 0;
    }
  }
  put(slot, 0x80, timer);
  put(slot, 0x84, duration);
  if (reserved) {
    ++quota.pending;
    ++quota.by_type[kind];
  }
  return result;
}
} // namespace comet::decomp
