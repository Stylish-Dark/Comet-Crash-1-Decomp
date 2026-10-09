#pragma once
#include <array>
#include <cstdint>
#include <span>
namespace comet::decomp {
// Arguments to the shared initializer at PPU 0xFECFC. Scalar names remain
// offset-based until their consumers establish game-domain meanings.
struct ArenaEntityCoreParameters {
  std::uint8_t type = 0, model_slot = 0, owner = 0, stage = 0;
  std::uint64_t quantity = 0;
  std::uint32_t flags = 0;
  std::array<float, 4> position{}, vector_40{};
  float scalar_58 = 0, scalar_3c = 0, scalar_38 = 0, scalar_64 = 0;
};
// Mutates only the fields written by 0xFECFC. Bytes remain big endian, matching
// the original entity bank and preserving untouched bytes in reused slots.
void initialize_arena_entity_core(std::span<std::uint8_t, 128>,
                                  const ArenaEntityCoreParameters &);
// Offset-based transitions called by constructors and initial-map upgrades.
// They do not change the packed owner/stage byte at +0x33.
void reset_arena_entity_upgrade_core(std::span<std::uint8_t, 128>,
                                     std::uint8_t level);
void settle_arena_entity_upgrade_core(std::span<std::uint8_t, 128>);
} // namespace comet::decomp
