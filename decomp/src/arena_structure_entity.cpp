#include "comet/arena_structure_entity.hpp"
#include "comet/arena_entity_core.hpp"
#include <bit>
namespace comet::decomp {
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
