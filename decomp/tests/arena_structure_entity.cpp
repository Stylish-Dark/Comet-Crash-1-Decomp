#include "comet/arena_structure_entity.hpp"
#include <stdexcept>
using namespace comet::decomp;
static void check(bool v) {
  if (!v)
    throw std::runtime_error("structure entity check failed");
}
static std::uint32_t word(const std::array<std::uint8_t, 256> &b, unsigned o) {
  return (std::uint32_t(b[o]) << 24) | (std::uint32_t(b[o + 1]) << 16) |
         (std::uint32_t(b[o + 2]) << 8) | b[o + 3];
}
int main() {
  std::array<std::uint8_t, 256> type20;
  type20.fill(0xa5);
  initialize_arena_type20_entity(type20, 1, {3, 4}, {80, 20});
  check(type20[0xf] == 20 && type20[0x6f] == 17 && type20[0x33] == 0x30);
  check(word(type20, 0x38) == 0x40800000 && word(type20, 0x64) == 0x42a00000);
  check(word(type20, 0x3c) == 0 && word(type20, 0x58) == 0x43480000);
  check(word(type20, 0x60) == 0x0001c28e && type20[0x7c] == 40 &&
        type20[0xe] == 0);
  check(word(type20, 0x10) == 2 && word(type20, 0x8c) == 0 &&
        type20[0x6e] == 1);
  check(word(type20, 0x80) == 0xa5a5a5a5 && word(type20, 0x88) == 0xa5a5a5a5);
  for (unsigned off = 0x90; off < 256; ++off)
    check(type20[off] == 0xa5);
  std::array<std::uint8_t, 256> bytes;
  bytes.fill(0xa5);
  initialize_arena_type21_entity(bytes, 2, {8, 12}, {50, 10, 25, 10});
  check(word(bytes, 0) == 0x41080000 && word(bytes, 8) == 0x41480000);
  check(bytes[0xf] == 21 && bytes[0x6f] == 11 && bytes[0x33] == 0x50);
  check(word(bytes, 0x38) == 0x40a00000 && word(bytes, 0x64) == 0x42480000);
  check(word(bytes, 0x3c) == 0x41d00000 && word(bytes, 0x58) == 0x43480000);
  check(word(bytes, 0x5c) == 0x3f800000 && word(bytes, 0x10) == 2);
  check(word(bytes, 0x60) == 0x0040c28f && bytes[0xe] == 0 &&
        bytes[0x7c] == 35);
  check(bytes[0x6e] == 1 && word(bytes, 0x74) == 0x38d1b717 &&
        word(bytes, 0x78) == 0);
  for (unsigned off = 0x80; off < 0x90; ++off)
    check(bytes[off] == 0);
  check(word(bytes, 0xa0) == 0 && word(bytes, 0xac) == 0x3f800000);
  check(word(bytes, 0xb0) == 0 && word(bytes, 0xb4) == 0 &&
        word(bytes, 0xb8) == 0x3e800000);
  check(word(bytes, 0xbc) == 0x42c80000 && word(bytes, 0xc4) == 0);
  check(bytes[0xcc] == 13 && bytes[0xcd] == 7 && bytes[0xce] == 255 &&
        bytes[0xcf] == 255);
  check(word(bytes, 0xd0) == 0 && word(bytes, 0xd4) == 0x4096cbe4 &&
        word(bytes, 0xd8) == 0x4016cbe4);
  check(bytes[0xdc] == 21 && bytes[0xdd] == 1 && bytes[0xde] == 255 &&
        bytes[0xdf] == 255);
  check(word(bytes, 0xe0) == 0x3e23d70a && word(bytes, 0xe4) == 0x3e99999a);
  check(word(bytes, 0xe8) == 0x3f733333 && word(bytes, 0xec) == 0xbf333333);
  // Reused slots retain fields outside the original constructor's writes.
  check(word(bytes, 0x90) == 0xa5a5a5a5 && word(bytes, 0x9c) == 0xa5a5a5a5);
  check(word(bytes, 0xf0) == 0xa5a5a5a5 && word(bytes, 0xfc) == 0xa5a5a5a5);
}
