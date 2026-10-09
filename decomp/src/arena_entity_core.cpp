#include "comet/arena_entity_core.hpp"
#include <bit>
#include <cstddef>
namespace comet::decomp {
namespace {
std::uint32_t read_word(std::span<std::uint8_t, 128> bytes, std::size_t off) {
  return (std::uint32_t(bytes[off]) << 24) |
         (std::uint32_t(bytes[off + 1]) << 16) |
         (std::uint32_t(bytes[off + 2]) << 8) | bytes[off + 3];
}
void write_word(std::span<std::uint8_t, 128> bytes, std::size_t off,
                std::uint32_t value) {
  for (unsigned i = 0; i < 4; ++i)
    bytes[off + i] = static_cast<std::uint8_t>(value >> (24 - 8 * i));
}
} // namespace
void initialize_arena_entity_core(std::span<std::uint8_t, 128> bytes,
                                  const ArenaEntityCoreParameters &p) {
  auto put_word = [&](std::size_t off, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
      bytes[off + i] = static_cast<std::uint8_t>(value >> (24 - 8 * i));
  };
  auto put_float = [&](std::size_t off, float value) {
    put_word(off, std::bit_cast<std::uint32_t>(value));
  };
  for (unsigned i = 0; i < 3; ++i) {
    put_float(4 * i, p.position[i]);
    put_float(0x40 + 4 * i, p.vector_40[i]);
    put_word(0x20 + 4 * i, 0);
  }
  put_float(0x2c, 1);
  bytes[0x0f] = p.type;
  put_word(0x10, 32);
  put_float(0x14, 1);
  put_float(0x18, 1);
  for (auto off : {0x1c, 0x1e, 0x6c}) {
    bytes[off] = 255;
    bytes[off + 1] = 255;
  }
  bytes[0x32] = static_cast<std::uint8_t>(p.quantity);
  bytes[0x33] =
      static_cast<std::uint8_t>(((p.owner & 7) << 5) | ((p.stage & 7) << 2));
  bytes[0x34] = bytes[0x35] = 0;
  put_float(0x38, p.scalar_38);
  put_float(0x3c, p.scalar_3c);
  put_word(0x50, 0);
  put_word(0x54, 0);
  put_float(0x58, p.scalar_58);
  put_word(0x5c, 0);
  put_word(0x60, p.flags | (p.stage <= 3 ? 2u : 6u));
  put_float(0x64, p.scalar_64);
  bytes[0x6f] = p.model_slot;
  // fcfid reads the signed 64-bit bit pattern; frsp precedes fmuls.
  const float quantity = static_cast<float>(
      static_cast<double>(std::bit_cast<std::int64_t>(p.quantity)));
  put_float(0x78, quantity * std::bit_cast<float>(0x3c23d70au));
  bytes[0x0e] = bytes[0x7c] = static_cast<std::uint8_t>(p.quantity);
  bytes[0x7d] = 150;
  bytes[0x7e] = 255;
  bytes[0x7f] = 0;
}
void reset_arena_entity_upgrade_core(std::span<std::uint8_t, 128> bytes,
                                     std::uint8_t level) {
  // 12869C rotates away bit 24 before setting bits 3 and 7.
  write_word(bytes, 0x60, (read_word(bytes, 0x60) & ~0x01000000u) | 0x88u);
  write_word(bytes, 0x74, 0x38d1b717u);
  write_word(bytes, 0x78, 0);
  bytes[0x6e] = level;
  bytes[0x0e] = 0;
  write_word(bytes, 0x68, 0);
}
void settle_arena_entity_upgrade_core(std::span<std::uint8_t, 128> bytes) {
  // 128C9C adds two unsigned halfword counters and stores the low halfword.
  const auto counters = read_word(bytes, 0x68);
  const auto sum =
      static_cast<std::uint16_t>((counters >> 16) + (counters & 0xffff));
  write_word(bytes, 0x68, std::uint32_t(sum) << 16);
  write_word(bytes, 0x60, (read_word(bytes, 0x60) & ~0x18u) | 0x01000000u);
  write_word(bytes, 0x74, 0);
  write_word(bytes, 0x78, 0x3f800000u);
  bytes[0x0e] = bytes[0x7c];
}
} // namespace comet::decomp
