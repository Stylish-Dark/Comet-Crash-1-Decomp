#include "comet/arena_unit_entity.hpp"
#include "comet/arena_entity_core.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
// Offset-based values from the nine 14B30 specialization arms. Domain names
// require their movement/weapon consumers; these are original float bits.
struct UnitValues {
  std::uint32_t scalar80, scalar84, scalar90, scalar9c, scalara0, scalara4,
      scalare8, scalar58, scalar64, flags, minimum78, fraction32;
  std::uint8_t quantity, stage, category, byte7d, word10;
};
constexpr std::array<UnitValues, 9> values{
    {{0x409ccccc, 0x3fcccccc, 0x3f7ae147, 0x3e4ccccc, 0x3f800000, 0x40000000,
      0x3f800000, 0x40800000, 0x42480000, 0x6000, 0x3f800000, 0x3df5c28f, 10, 5,
      1, 140, 96},
     {0x40d00000, 0x3fa66666, 0x3f7ae147, 0x3e800000, 0x3f800000, 0x40000000,
      0x3f800000, 0x41200000, 0x432a0000, 0x6000, 0x3f800000, 0x3e199999, 10, 5,
      1, 110, 96},
     {0x3f800000, 0x3f999999, 0x3eb33333, 0x3e4ccccc, 0x3f4ccccc, 0x40000000,
      0x3f800000, 0x40c00000, 0x42c80000, 0x4000, 0x3f800000, 0, 15, 6, 2, 135,
      64},
     {0x40eccccc, 0x40899999, 0x3f733333, 0x3e4ccccc, 0x3f800000, 0x40000000,
      0x3f800000, 0x41a00000, 0x43a00000, 0x6000, 0x3f800000, 0x3e3851eb, 10, 5,
      3, 120, 96},
     {0x40600000, 0x400ccccc, 0x3f7ae147, 0, 0x42480000, 0x42480000, 0x3e999999,
      0x430c0000, 0x44160000, 0x4000, 0x3f800000, 0x3e570a3d, 10, 5, 2, 100,
      96},
     {0x40600000, 0x3f000000, 0x3f7ae147, 0x3ecccccc, 0x40800000, 0x40a00000,
      0x3e999999, 0x41f00000, 0x44960000, 0x4000, 0x3f800000, 0x3e800000, 10, 5,
      2, 100, 96},
     {0x40900000, 0x3f800000, 0x3f7ae147, 0x3e999999, 0x40400000, 0x40a00000,
      0x3f800000, 0x41a00000, 0x44160000, 0x0a004000, 0x3f800000, 0x3e6e631d,
      10, 5, 2, 120, 96},
     {0x40400000, 0x3ecccccc, 0x3f733333, 0x40400000, 0x40800000, 0x40a00000,
      0x3dcccccc, 0x42200000, 0x43af0000, 0x5000, 0x3f800000, 0x3eb33333, 10, 5,
      3, 100, 96},
     {0x40400000, 0x3eb33333, 0x3f733333, 0x40400000, 0x40800000, 0x40a00000,
      0x3dcccccc, 0x428c0000, 0x44fa0000, 0x4000, 0x3fc00000, 0, 45, 6, 3, 100,
      64}}};
float f(std::uint32_t bits) { return std::bit_cast<float>(bits); }
float multiply(float a, float b) {
  volatile float v = a * b;
  return v;
}
float add(float a, float b) {
  volatile float v = a + b;
  return v;
}
struct Rounding {
  int previous = std::fegetround();
  Rounding() {
    if (previous < 0 || std::fesetround(FE_TOWARDZERO) != 0)
      throw std::runtime_error("unit rounding unavailable");
  }
  ~Rounding() { std::fesetround(previous); }
};
} // namespace
bool initialize_arena_unit_template(std::span<std::uint8_t, 256> slot,
                                    std::uint8_t kind, std::uint8_t owner,
                                    const std::array<float, 4> &position,
                                    ArenaRandom &random) {
  if (kind < 40 || kind > 48 || owner > 3)
    return false;
  for (float p : position)
    if (!std::isfinite(p) || p < -32768 || p >= 32768)
      return false;
  Rounding rounding;
  const auto &v = values[kind - 40];
  auto put = [&](unsigned off, std::uint32_t word) {
    for (unsigned i = 0; i < 4; ++i)
      slot[off + i] = word >> (24 - 8 * i);
  };
  auto half = [&](unsigned off, std::uint16_t value) {
    slot[off] = value >> 8;
    slot[off + 1] = value;
  };
  // One shuffled RNG draw, CSFLT under SPU rounding, then original reciprocal
  // refinement for divisor float_bits(4E7FFFFF), followed by the angle
  // multiply.
  volatile float converted = static_cast<float>(random.next());
  const float estimate = multiply(converted, f(0x30800000));
  const float residual = std::fma(-estimate, f(0x4e7fffff), converted);
  const float ratio = std::fma(residual, f(0x30800000), estimate);
  const float angle = std::fma(ratio, f(0x40c90fda), 0.0f);
  ArenaEntityCoreParameters core;
  core.type = kind;
  core.model_slot = kind - 38;
  core.owner = owner;
  core.stage = v.stage;
  core.quantity = v.quantity;
  core.flags = v.flags;
  core.position = position;
  core.scalar_58 = f(v.scalar58);
  core.scalar_38 = core.scalar_64 = f(v.scalar64);
  initialize_arena_entity_core(slot.first<128>(), core);
  put(0x10, v.word10);
  slot[0x33] |= v.category;
  slot[0x7d] = v.byte7d;
  put(0x78, v.minimum78);
  put(0x50,
      std::bit_cast<std::uint32_t>(multiply(f(v.scalar64), f(v.scalar84))));
  if (v.fraction32)
    slot[0x32] = static_cast<std::uint8_t>(
        multiply(add(f(v.fraction32), f(0x3ba3d70a)), 100.0f));
  put(0x80, v.scalar80);
  put(0x84, v.scalar84);
  put(0x88,
      std::bit_cast<std::uint32_t>(multiply(f(v.scalar84), f(v.scalar84))));
  put(0x8c, 0);
  put(0x90, v.scalar90);
  put(0x94, 0x40400000);
  put(0x98, 0);
  put(0x9c, v.scalar9c);
  put(0xa0, v.scalara0);
  put(0xa4, v.scalara4);
  put(0xa8, std::bit_cast<std::uint32_t>(angle));
  put(0xac, 0);
  half(0xb4, 0xffff);
  half(0xb6, 0xffff);
  half(0xb8, 0);
  put(0xbc, 0);
  put(0xc0, 0);
  put(0xc4, 0);
  if (kind == 47) {
    half(0xb0, 20);
    slot[0xb9] = 1;
  }
  const auto x =
      static_cast<std::uint16_t>(static_cast<std::int32_t>(position[0]));
  const auto z =
      static_cast<std::uint16_t>(static_cast<std::int32_t>(position[2]));
  half(0xc8, x);
  half(0xca, z);
  half(0xcc, x);
  half(0xce, z);
  put(0xe0, 0);
  put(0xe4, 0);
  put(0xe8, v.scalare8);
  put(0xec, 0);
  return true;
}
} // namespace comet::decomp
