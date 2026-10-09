#pragma once
#include "comet/arena_state.hpp"
namespace comet::decomp {
struct ArenaType20Tuning {
  std::int16_t scalar_64 = 0, scalar_38_denominator = 1;
};
// Complete opcode-20 slot writes at D7090..D7134, including both leaf calls.
void initialize_arena_type20_entity(std::span<std::uint8_t, 256>,
                                    std::uint8_t owner, ArenaGridPosition,
                                    const ArenaType20Tuning &);
// Values read from mutable bootstrap tables by the opcode-21 constructor.
// Names retain destination offsets while gameplay semantics are unproved.
struct ArenaType21Tuning {
  std::int16_t scalar_64 = 0, scalar_38_denominator = 1;
  std::uint8_t scalar_b8_count = 0, scalar_5c_count = 0;
};
// Mutable table reads shared by the opcode 20..28 constructors. The byte
// mapping depends on opcode and is documented in arena-entity-construction.md.
struct ArenaStructureTableValues {
  std::int16_t scalar_64 = 0, scalar_38_denominator = 1;
  std::uint8_t first_byte = 0, second_byte = 0;
  float scalar_84 = 0;
};
// Supports 20..26 and 28. Opcode 25 requires random; other arms ignore it.
// Unsupported opcodes or missing required random leave the slot unchanged.
bool initialize_arena_structure_entity(std::span<std::uint8_t, 256>,
                                       std::uint8_t opcode, std::uint8_t owner,
                                       ArenaGridPosition,
                                       const ArenaStructureTableValues &,
                                       ArenaRandom *random = nullptr);
// Entire opcode-21 entity-slot initialization at D7138..D72E4, including
// FECFC and 12869C. Allocation, economy and cell-list insertion are external.
void initialize_arena_type21_entity(std::span<std::uint8_t, 256>,
                                    std::uint8_t owner, ArenaGridPosition,
                                    const ArenaType21Tuning &);
} // namespace comet::decomp
