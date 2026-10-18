#pragma once
#include "comet/arena_entity_storage.hpp"
namespace comet::decomp {
enum class ArenaImpulseDelta { Applied, Suppressed, Invalid };
// Ordinary incoming-message vector, without original delayed-message modifiers.
// Ordered RTZ accumulation is atomic across XYZ. W must be zero.
bool accumulate_arena_impulse(std::array<float,4>&,const std::array<float,4>&);
// FE5D8 9EE0..9F80: flag4000 gates velocity response; impulse length is capped
// at8, then scaled by inverse mass58 and byte7D/100. Preserves metadata W.
// Finite normal-or-zero inputs, |impulse XYZ|<=1e6, positive mass<=1e6.
// Invalid input preserves the slot; caller consumes the pending vector once.
ArenaImpulseDelta apply_arena_impulse(ArenaEntitySlot&,const std::array<float,4>&);
}
