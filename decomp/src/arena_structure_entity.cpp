#include "comet/arena_structure_entity.hpp"
#include "comet/arena_entity_core.hpp"
#include <bit>
#include <cmath>
namespace comet::decomp {
namespace {
float constant(std::uint32_t bits) { return std::bit_cast<float>(bits); }
float multiply(float a, float b) {
  volatile float result = a * b;
  return result;
}
// 193CB4's finite, nonnegative angle path. Gate draws produce [0,3.14].
// Preserve its float rounding and fused polynomial operations rather than
// substituting the host's trigonometric library.
float gate_trig(float angle, unsigned mode) {
  if (angle == 0)
    return mode == 0 ? angle : 1.0f;
  const float scaled = multiply(angle, constant(0x3f22f983));
  const auto quadrant = static_cast<unsigned>(scaled + 0.5f);
  const float n = static_cast<float>(quadrant);
  float reduced = std::fma(n, constant(0xbfc90fda), angle);
  reduced = std::fma(n, constant(0xb3a22169), reduced);
  const auto selector = quadrant + mode;
  float value;
  if (reduced <= -constant(0x39800000) ||
      reduced >= constant(0x39800000)) {
    const float square = multiply(reduced, reduced);
    if (selector & 1) {
      float p = std::fma(square, constant(0xbab24993), constant(0x3d2aa036));
      p = std::fma(square, p, -constant(0x3effffdf));
      value = std::fma(square, p, 1.0f);
    } else {
      const float cube = multiply(reduced, square);
      float p = std::fma(square, constant(0xb94c8c6e), constant(0x3c088342));
      p = std::fma(square, p, -constant(0x3e2aaaa1));
      value = std::fma(cube, p, reduced);
    }
  } else {
    value = (selector & 1) ? 1.0f : reduced;
  }
  return (selector & 2) ? -value : value;
}
void initialize_gate(std::span<std::uint8_t, 256> bytes, std::uint8_t owner,
                     ArenaGridPosition grid, const ArenaStructureTableValues &t,
                     ArenaRandom &random) {
  ArenaEntityCoreParameters p;
  p.type = 25;
  p.model_slot = 35;
  p.owner = owner;
  p.stage = 4;
  p.quantity = 15;
  p.flags = 0x00014200;
  p.position = {static_cast<float>(grid.x) + 0.5f, 0,
                static_cast<float>(grid.z) + 0.5f, 0};
  p.scalar_58 = 200;
  p.scalar_64 = static_cast<float>(t.scalar_64);
  p.scalar_38 = p.scalar_64 / static_cast<float>(t.scalar_38_denominator);
  initialize_arena_entity_core(bytes.first<128>(), p);
  auto word = [&](unsigned off, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
      bytes[off + i] = static_cast<std::uint8_t>(value >> (24 - 8 * i));
  };
  auto scalar = [&](unsigned off, float value) {
    word(off, std::bit_cast<std::uint32_t>(value));
  };
  const float unit =
      multiply(static_cast<float>(random.next()), constant(0x30800000));
  const float angle = std::fma(unit, constant(0x4048f5c3), 0.0f);
  scalar(0x80, angle);
  word(0x84, 0);
  scalar(0x88, angle);
  word(0x8c, 0);
  const float dx = multiply(gate_trig(angle, 0), constant(0x3eb33333));
  const float dz = multiply(gate_trig(angle, 1), constant(0x3eb33333));
  scalar(0x90, p.position[0] + dx);
  word(0x94, 0);
  scalar(0x98, p.position[2] + dz);
  word(0x9c, 0);
  word(0x10, 2);
  word(0x5c, 0);
  reset_arena_entity_upgrade_core(bytes.first<128>(), 5);
}
} // namespace

bool initialize_arena_structure_entity(std::span<std::uint8_t, 256> bytes,
                                       std::uint8_t opcode, std::uint8_t owner,
                                       ArenaGridPosition grid,
                                       const ArenaStructureTableValues &t,
                                       ArenaRandom *random) {
  if (opcode == 25) {
    if (!random)
      return false;
    initialize_gate(bytes, owner, grid, t, *random);
    return true;
  }
  if (opcode == 20) {
    initialize_arena_type20_entity(bytes, owner, grid,
                                   {t.scalar_64, t.scalar_38_denominator});
    return true;
  }
  if (opcode == 21) {
    initialize_arena_type21_entity(
        bytes, owner, grid,
        {t.scalar_64, t.scalar_38_denominator, t.first_byte, t.second_byte});
    return true;
  }
  if (opcode != 22 && opcode != 23 && opcode != 24 && opcode != 26 &&
      opcode != 28)
    return false;
  ArenaEntityCoreParameters p;
  p.type = opcode;
  p.owner = owner;
  p.stage = 4;
  p.model_slot = opcode == 22   ? 16
                 : opcode == 23 ? 14
                 : opcode == 24 ? 18
                 : opcode == 26 ? 24
                                : 25;
  p.quantity = opcode == 23 ? 38 : 40;
  p.flags = opcode >= 26 ? 0x0040c200 : 0x0040c201;
  p.position = {static_cast<float>(grid.x) + 0.5f, 0,
                static_cast<float>(grid.z) + 0.5f, 0};
  p.scalar_58 = 200;
  p.scalar_3c = opcode == 23 ? 25.0f : 0.0f;
  p.scalar_64 = static_cast<float>(t.scalar_64);
  p.scalar_38 = p.scalar_64 / static_cast<float>(t.scalar_38_denominator);
  initialize_arena_entity_core(bytes.first<128>(), p);
  auto word = [&](unsigned off, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
      bytes[off + i] = static_cast<std::uint8_t>(value >> (24 - 8 * i));
  };
  auto scalar = [&](unsigned off, float value) {
    word(off, std::bit_cast<std::uint32_t>(value));
  };
  if (opcode < 26)
    word(0x10, 2);
  if (opcode < 26) {
    for (unsigned off = 0x80; off < 0x90; ++off)
      bytes[off] = 0;
    const auto count = opcode == 23 ? t.second_byte : t.first_byte;
    scalar(0x5c, static_cast<float>(count) * std::bit_cast<float>(0x3dcccccdu));
  }
  if (opcode == 22) {
    bytes[0x90] = bytes[0x91] = 255;
    word(0xb0, 0x42a00000);
    word(0xb4, 0x3f800000);
    word(0xb8, 1);
    word(0xbc, 0);
    word(0xc0, 0);
    word(0xc4, 0x3f800000);
    word(0xc8, 0xbf800000);
  } else if (opcode == 23 || opcode == 24) {
    word(0xa0, 0);
    word(0xa4, 0);
    word(0xa8, 0);
    word(0xac, 0x3f800000);
    word(0xb0, 0);
    word(0xb4, 0);
    word(0xbc, 0x42c80000);
    word(0xc0, 0);
    word(0xc4, 0);
    word(0xc8, 0);
    word(0xd0, 0);
    bytes[0xce] = bytes[0xcf] = bytes[0xde] = bytes[0xdf] = 255;
    word(0xe0, 0x3dcccccd);
    if (opcode == 23) {
      scalar(0xb8, static_cast<float>(t.first_byte) *
                       std::bit_cast<float>(0x3c23d70au));
      bytes[0xcc] = bytes[0xcd] = 15;
      bytes[0xdc] = 23;
      bytes[0xdd] = 1;
      word(0xd4, 0x40c90fdb);
      word(0xd8, 0x40490fdb);
      word(0xe4, 0x3e4ccccd);
      word(0xe8, 0x3f7d70a4);
      word(0xec, 0xbe99999a);
      word(0xf0, 110);
    } else {
      word(0xb8, 0x40000000);
      bytes[0xcc] = 20;
      bytes[0xcd] = 30;
      bytes[0xdc] = 29;
      bytes[0xdd] = 50;
      word(0xd4, 0x3fc90fdb);
      word(0xd8, 0x3fc90fdb);
      // f30 remains the pre-switch grid-position constant 0.5 in this arm.
      word(0xe4, 0x3f000000);
      word(0xe8, 0x3f59999a);
      word(0xec, 0x3f000000);
      scalar(0xf0, static_cast<float>(t.second_byte));
      word(0xf4, 0x3ec28f5c);
      word(0xf8, 0x3dcccccd);
      word(0xfc, 0x3f800000);
    }
  } else {
    word(0x80, 0);
    scalar(0x84, t.scalar_84);
    word(0x88, 0);
    word(0xa0, 0);
    word(0xcc, 0);
    bytes[0xa8] = bytes[0xaa] = bytes[0xab] = 0;
    word(0xb8, opcode == 26 ? 40 : 44);
    word(0xbc, opcode == 26 ? 2 : 6);
    word(0xc0, opcode == 26 ? 1 : 4);
    word(0xc4, 4);
    word(0xc8, opcode == 26 ? 4 : 5);
    bytes[0x96] = bytes[0x97] = 255;
  }
  reset_arena_entity_upgrade_core(bytes.first<128>(), opcode == 28   ? 5
                                                      : opcode == 26 ? 2
                                                                     : 1);
  return true;
}
void initialize_arena_type20_entity(std::span<std::uint8_t, 256> bytes,
                                    std::uint8_t owner, ArenaGridPosition grid,
                                    const ArenaType20Tuning &tuning) {
  ArenaEntityCoreParameters p;
  p.type = 20;
  p.model_slot = 17;
  p.owner = owner;
  p.stage = 4;
  p.quantity = 40;
  p.flags = 0x0001c200;
  p.position = {static_cast<float>(grid.x) + 0.5f, 0,
                static_cast<float>(grid.z) + 0.5f, 0};
  p.scalar_58 = 200;
  p.scalar_64 = static_cast<float>(tuning.scalar_64);
  p.scalar_38 = p.scalar_64 / static_cast<float>(tuning.scalar_38_denominator);
  initialize_arena_entity_core(bytes.first<128>(), p);
  bytes[0x10] = bytes[0x11] = bytes[0x12] = 0;
  bytes[0x13] = 2;
  for (unsigned off = 0x8c; off < 0x90; ++off)
    bytes[off] = 0;
  reset_arena_entity_upgrade_core(bytes.first<128>(), 1);
}
void initialize_arena_type21_entity(std::span<std::uint8_t, 256> bytes,
                                    std::uint8_t owner, ArenaGridPosition grid,
                                    const ArenaType21Tuning &tuning) {
  ArenaEntityCoreParameters p;
  p.type = 21;
  p.model_slot = 11;
  p.owner = owner;
  p.stage = 4;
  p.quantity = 35;
  p.flags = 0x0040c201;
  p.position = {static_cast<float>(grid.x) + 0.5f, 0,
                static_cast<float>(grid.z) + 0.5f, 0};
  p.scalar_58 = 200;
  p.scalar_3c = 26;
  p.scalar_64 = static_cast<float>(tuning.scalar_64);
  p.scalar_38 = p.scalar_64 / static_cast<float>(tuning.scalar_38_denominator);
  initialize_arena_entity_core(bytes.first<128>(), p);
  auto put_word = [&](unsigned off, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
      bytes[off + i] = static_cast<std::uint8_t>(value >> (24 - 8 * i));
  };
  auto put_float = [&](unsigned off, float value) {
    put_word(off, std::bit_cast<std::uint32_t>(value));
  };
  for (unsigned off = 0x80; off < 0x90; ++off)
    bytes[off] = 0;
  put_word(0xa0, 0);
  put_word(0xa4, 0);
  put_word(0xa8, 0);
  put_word(0xac, 0x3f800000);
  put_word(0xb0, 0);
  put_word(0xb4, 0);
  put_float(0xb8, static_cast<float>(tuning.scalar_b8_count) *
                      std::bit_cast<float>(0x3c23d70au));
  put_word(0xbc, 0x42c80000);
  put_word(0xc0, 0);
  put_word(0xc4, 0);
  put_word(0xc8, 0);
  bytes[0xcc] = 13;
  bytes[0xcd] = 7;
  bytes[0xce] = bytes[0xcf] = 255;
  put_word(0xd0, 0);
  put_word(0xd4, 0x4096cbe4);
  put_word(0xd8, 0x4016cbe4);
  bytes[0xdc] = 21;
  bytes[0xdd] = 1;
  bytes[0xde] = bytes[0xdf] = 255;
  put_word(0xe0, 0x3e23d70a);
  put_word(0xe4, 0x3e99999a);
  put_word(0xe8, 0x3f733333);
  put_word(0xec, 0xbf333333);
  put_float(0x5c, static_cast<float>(tuning.scalar_5c_count) *
                      std::bit_cast<float>(0x3dcccccdu));
  put_word(0x10, 2);
  reset_arena_entity_upgrade_core(bytes.first<128>(), 1);
}
} // namespace comet::decomp
