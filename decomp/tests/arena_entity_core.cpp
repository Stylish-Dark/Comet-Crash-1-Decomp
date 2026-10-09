#include "comet/arena_entity_core.hpp"
#include <bit>
#include <stdexcept>
using namespace comet::decomp;
static void check(bool value) {
  if (!value)
    throw std::runtime_error("entity core check failed");
}
static std::uint32_t word(const std::array<std::uint8_t, 128> &b, int off) {
  return (std::uint32_t(b[off]) << 24) | (std::uint32_t(b[off + 1]) << 16) |
         (std::uint32_t(b[off + 2]) << 8) | b[off + 3];
}
int main() {
  std::array<std::uint8_t, 128> bytes;
  bytes.fill(0xa5);
  ArenaEntityCoreParameters p;
  p.type = 21;
  p.model_slot = 11;
  p.owner = 2;
  p.stage = 4;
  p.quantity = 40;
  p.flags = 0x0040c200;
  p.position = {2.5f, 0, 3.5f, 123};
  p.vector_40 = {1, 2, 3, 456};
  p.scalar_58 = 7;
  p.scalar_3c = 8;
  p.scalar_38 = 9;
  p.scalar_64 = 10;
  initialize_arena_entity_core(bytes, p);
  check(word(bytes, 0) == 0x40200000 && word(bytes, 8) == 0x40600000);
  // vsel preserves the fourth word, not the input vector's W.
  check(word(bytes, 0xc) == 0xa5a52815 && word(bytes, 0x4c) == 0xa5a5a5a5);
  check(word(bytes, 0x20) == 0 && word(bytes, 0x2c) == 0x3f800000);
  check(bytes[0x32] == 40 && bytes[0x33] == 0x50);
  check(word(bytes, 0x10) == 32 && word(bytes, 0x14) == 0x3f800000);
  check(word(bytes, 0x18) == 0x3f800000 && bytes[0x1c] == 255 &&
        bytes[0x1f] == 255);
  check(bytes[0x30] == 0xa5 && bytes[0x31] == 0xa5 && bytes[0x36] == 0xa5);
  check(word(bytes, 0x38) == 0x41100000 && word(bytes, 0x3c) == 0x41000000);
  check(word(bytes, 0x58) == 0x40e00000 && word(bytes, 0x64) == 0x41200000);
  check(word(bytes, 0x60) == 0x0040c206 && bytes[0x6f] == 11);
  check(word(bytes, 0x78) == 0x3ecccccc); // frsp(40) * float 0x3c23d70a
  check(bytes[0x7c] == 40 && bytes[0x7d] == 150 && bytes[0x7e] == 255 &&
        bytes[0x7f] == 0);
  // Comparison uses the unmasked stage byte, while packing uses low 3 bits.
  p.owner = 255;
  p.stage = 8;
  p.quantity = 257;
  p.flags = 0;
  initialize_arena_entity_core(bytes, p);
  check(bytes[0x33] == 0xe0 && bytes[0x32] == 1 && word(bytes, 0x60) == 6);
  p.stage = 3;
  initialize_arena_entity_core(bytes, p);
  check(bytes[0x33] == 0xec && word(bytes, 0x60) == 2);
  bytes.fill(0xa5);
  reset_arena_entity_upgrade_core(bytes, 3);
  check(word(bytes, 0x60) == 0xa4a5a5ad);
  check(bytes[0x6e] == 3 && bytes[0xe] == 0 && word(bytes, 0x78) == 0);
  check(word(bytes, 0x74) == 0x38d1b717);
  check(bytes[0x68] == 0 && bytes[0x69] == 0 && bytes[0x6a] == 0 &&
        bytes[0x6b] == 0);
  check(bytes[0x33] == 0xa5 && bytes[0x7c] == 0xa5);
  bytes[0x68] = 255;
  bytes[0x69] = 255;
  bytes[0x6a] = 0;
  bytes[0x6b] = 2;
  settle_arena_entity_upgrade_core(bytes);
  check(bytes[0x68] == 0 && bytes[0x69] == 1); // halfword wrap
  check(bytes[0x6a] == 0 && bytes[0x6b] == 0 && word(bytes, 0x74) == 0);
  check(word(bytes, 0x78) == 0x3f800000 && bytes[0xe] == 0xa5);
  check(word(bytes, 0x60) == 0xa5a5a5a5 && bytes[0x33] == 0xa5);
}
