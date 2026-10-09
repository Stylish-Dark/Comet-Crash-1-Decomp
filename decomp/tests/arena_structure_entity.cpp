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
  std::array<std::uint8_t, 256> extended;
  extended.fill(0xa5);
  ArenaStructureTableValues tuning{100, 50, 10, 20, 7.5f};
  check(initialize_arena_structure_entity(extended, 22, 2, {3, 4}, tuning));
  check(extended[0xf] == 22 && extended[0x6f] == 16 && extended[0x7c] == 40);
  check(word(extended, 0x60) == 0x0040c28f && extended[0x6e] == 1);
  check(extended[0x90] == 255 && extended[0x91] == 255);
  check(word(extended, 0xb0) == 0x42a00000 &&
        word(extended, 0xb4) == 0x3f800000);
  check(extended[0xb8] == 0 && extended[0xb9] == 0 && extended[0xba] == 0 &&
        extended[0xbb] == 1);
  check(word(extended, 0xc0) == 0 && word(extended, 0xc4) == 0x3f800000 &&
        word(extended, 0xc8) == 0xbf800000);
  check(word(extended, 0x5c) == 0x3f800000 &&
        word(extended, 0x98) == 0xa5a5a5a5);
  extended.fill(0xa5);
  check(initialize_arena_structure_entity(extended, 23, 2, {3, 4}, tuning));
  check(extended[0x6f] == 14 && extended[0x7c] == 38 &&
        word(extended, 0x3c) == 0x41c80000);
  check(extended[0xcc] == 15 && extended[0xcd] == 15 && extended[0xdc] == 23 &&
        extended[0xdd] == 1);
  check(word(extended, 0xd4) == 0x40c90fdb &&
        word(extended, 0xd8) == 0x40490fdb);
  check(word(extended, 0xe0) == 0x3dcccccd &&
        word(extended, 0xe4) == 0x3e4ccccd);
  check(word(extended, 0xe8) == 0x3f7d70a4 &&
        word(extended, 0xec) == 0xbe99999a && word(extended, 0xf0) == 110);
  check(word(extended, 0x5c) == 0x40000000);
  extended.fill(0xa5);
  check(initialize_arena_structure_entity(extended, 24, 2, {3, 4}, tuning));
  check(extended[0x6f] == 18 && extended[0x7c] == 40);
  check(word(extended, 0xb8) == 0x40000000 && extended[0xcc] == 20 &&
        extended[0xcd] == 30);
  check(extended[0xdc] == 29 && extended[0xdd] == 50);
  check(word(extended, 0xe4) == 0x3f000000 &&
        word(extended, 0xec) == 0x3f000000);
  check(word(extended, 0xf0) == 0x41a00000 &&
        word(extended, 0xf4) == 0x3ec28f5c);
  check(word(extended, 0xf8) == 0x3dcccccd &&
        word(extended, 0xfc) == 0x3f800000);
  extended.fill(0xa5);
  check(initialize_arena_structure_entity(extended, 26, 2, {3, 4}, tuning));
  check(extended[0x6f] == 24 && extended[0x6e] == 2 &&
        word(extended, 0x60) == 0x0040c28e);
  check(word(extended, 0x10) == 32); // D788C arm preserves FECFC's default.
  check(word(extended, 0x84) == 0x40f00000 && word(extended, 0x88) == 0 &&
        word(extended, 0x8c) == 0xa5a5a5a5);
  check(extended[0x96] == 255 && extended[0x97] == 255 && extended[0xa8] == 0 &&
        extended[0xa9] == 0xa5 && extended[0xaa] == 0 && extended[0xab] == 0);
  check(word(extended, 0xb8) == 40 && word(extended, 0xbc) == 2 &&
        word(extended, 0xc0) == 1);
  check(word(extended, 0xc4) == 4 && word(extended, 0xc8) == 4 &&
        word(extended, 0xcc) == 0);
  extended.fill(0xa5);
  check(initialize_arena_structure_entity(extended, 28, 2, {3, 4}, tuning));
  check(extended[0xf] == 28 && extended[0x6f] == 25 && extended[0x6e] == 5);
  check(word(extended, 0x10) == 32 && word(extended, 0x84) == 0x40f00000);
  check(word(extended, 0xb8) == 44 && word(extended, 0xbc) == 6);
  check(word(extended, 0xc0) == 4 && word(extended, 0xc4) == 4 &&
        word(extended, 0xc8) == 5);
  check(word(extended, 0x8c) == 0xa5a5a5a5 && extended[0xa9] == 0xa5);
  check(extended[0x96] == 255 && extended[0x97] == 255);
  auto untouched = extended;
  check(!initialize_arena_structure_entity(extended, 25, 2, {3, 4}, tuning) &&
        extended == untouched);
  ArenaRandom gate_random(1);
  extended.fill(0xa5);
  check(initialize_arena_structure_entity(extended, 25, 2, {3, 4}, tuning,
                                         &gate_random));
  check(extended[0xf] == 25 && extended[0x6f] == 35 && extended[0x6e] == 5);
  check(extended[0x7c] == 15 && word(extended, 0x60) == 0x0001428e);
  check(word(extended, 0x10) == 2 && word(extended, 0x84) == 0 &&
        word(extended, 0x8c) == 0 && word(extended, 0x5c) == 0);
  check(word(extended, 0x80) == word(extended, 0x88));
  check(word(extended, 0x94) == 0 && word(extended, 0x9c) == 0);
  check(gate_random.next() == 135115861); // Exactly one random draw consumed.
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
