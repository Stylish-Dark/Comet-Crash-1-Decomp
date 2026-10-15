#pragma once
#include "comet/arena_entity_storage.hpp"
#include <optional>
namespace comet::decomp {
// FE5D8 16140..162AC with the single phase subtraction at 1801C..18028.
// Supports stage-5 kind 45, phase in [0, float_bits(40C90FDA)] and elapsed
// in [0, 3]. Positive +8C takes the original cooldown path (16F70..16FFC),
// updating +8C/+BB and preserving phase; otherwise updates +98/+BB. Returns
// the original context +18 scalar. The caller persists that scalar separately.
std::optional<float> advance_arena_converter_activation(ArenaEntitySlot &,float elapsed);
}
