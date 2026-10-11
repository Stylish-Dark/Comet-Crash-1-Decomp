#include "comet/arena_construction_progress.hpp"
#include "comet/arena_entity_core.hpp"
#include "comet/arena_structure_entity.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace comet::decomp {
namespace {
std::uint32_t word(std::span<const std::uint8_t, 256> slot, unsigned offset) {
  std::uint32_t v = 0;
  for (unsigned i = 0; i < 4; ++i)
    v = (v << 8) | slot[offset + i];
  return v;
}
void put(std::span<std::uint8_t, 256> slot, unsigned offset,
         std::uint32_t value) {
  for (unsigned i = 0; i < 4; ++i)
    slot[offset + i] = static_cast<std::uint8_t>(value >> (24 - 8 * i));
}
float scalar(std::span<const std::uint8_t, 256> slot, unsigned offset) {
  return std::bit_cast<float>(word(slot, offset));
}
float normal(float value) {
  return std::fpclassify(value) == FP_SUBNORMAL ? 0.0f : value;
}
struct ConstructionRounding {
  int previous = std::fegetround();
  ConstructionRounding() {
    if (previous < 0 || std::fesetround(FE_TOWARDZERO) != 0)
      throw std::runtime_error("construction rounding unavailable");
  }
  ~ConstructionRounding() { std::fesetround(previous); }
};
float multiply(float a, float b) {
  volatile float value = normal(a) * normal(b);
  return normal(value);
}
float add(float a, float b) {
  volatile float value = normal(a) + normal(b);
  return normal(value);
}
float fused(float a, float b, float c) {
  return normal(std::fma(normal(a), normal(b), normal(c)));
}
} // namespace
bool arena_structure_ready(std::span<const std::uint8_t, 256> slot) {
  return arena_structure_initial_level(slot[15]) && !(word(slot, 0x60) & 24) &&
         (word(slot, 0x60) & 0x01000000);
}
ArenaConstructionProgress
advance_arena_structure_construction(std::span<std::uint8_t, 256> slot,
                                     float elapsed) {
  if (!std::isfinite(elapsed) || elapsed < 0)
    throw std::invalid_argument("invalid construction elapsed time");
  if (!arena_structure_initial_level(slot[15]) || slot[0x6e] < 1 ||
      slot[0x6e] > 5)
    return ArenaConstructionProgress::Unsupported;
  auto flags = word(slot, 0x60);
  if (!(flags & 8))
    return ArenaConstructionProgress::Idle;
  if (flags & 96)
    return ArenaConstructionProgress::Unsupported;
  const float progress = scalar(slot, 0x74), health = scalar(slot, 0x38),
              maximum = scalar(slot, 0x64);
  if (!std::isfinite(progress) || progress < 0 || !std::isfinite(health) ||
      !std::isfinite(maximum))
    return ArenaConstructionProgress::Unsupported;
  float updated, updated_health, quantity;
  {
    ConstructionRounding rounding;
    // The original job's 50-byte duration table at 20BE0 is 2 for all ten
    // structure types and five levels. FREST(2) followed by FI gives this
    // reciprocal seed. Preserve both original refinement operations rather
    // than replacing the sequence with elapsed / 2.
    constexpr float reciprocal = std::bit_cast<float>(0x3efff800u);
    const float estimate = multiply(elapsed, reciprocal);
    const float residual = fused(-estimate, 2.0f, elapsed);
    const float increment = fused(residual, reciprocal, estimate);
    updated = add(increment, progress);
    updated_health = fused(increment, maximum, health);
    if (updated_health > maximum)
      updated_health = maximum;
    quantity = multiply(updated, static_cast<float>(slot[0x7c]));
  }
  if (!std::isfinite(updated) || !std::isfinite(updated_health))
    return ArenaConstructionProgress::Unsupported;
  flags &= ~128u;
  put(slot, 0x60, flags);
  put(slot, 0x74, std::bit_cast<std::uint32_t>(updated));
  put(slot, 0x78, std::bit_cast<std::uint32_t>(updated));
  put(slot, 0x38, std::bit_cast<std::uint32_t>(updated_health));
  const auto count =
      quantity <= 0 ? 0u
      : quantity >=
              static_cast<float>(std::numeric_limits<std::uint32_t>::max())
          ? std::numeric_limits<std::uint32_t>::max()
          : static_cast<std::uint32_t>(quantity);
  slot[14] = static_cast<std::uint8_t>(count);
  if (updated < 1)
    return ArenaConstructionProgress::Building;
  settle_arena_entity_upgrade_core(slot.first<128>());
  return ArenaConstructionProgress::Ready;
}
} // namespace comet::decomp
