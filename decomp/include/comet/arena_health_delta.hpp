#pragma once
#include "comet/arena_entity_storage.hpp"
namespace comet::decomp {
enum class ArenaHealthDelta { Applied, Suppressed, Invalid };
// FE5D8 17F0: signed pending-health accumulation. Negative values damage;
// positive values heal. Invalid native numbers/overflow preserve the sum.
bool accumulate_arena_health_delta(float &accumulator,float delta);
// FE5D8 9ED0..9EDC/A1D8..A210: flag 0x00100000 suppresses the write;
// otherwise health +38 becomes min(maximum +64, health + delta), without a
// lower clamp. Preserves every other slot byte and restores caller rounding.
// Nonfinite/subnormal native operands are rejected transactionally.
ArenaHealthDelta apply_arena_health_delta(ArenaEntitySlot &,float delta);
}
