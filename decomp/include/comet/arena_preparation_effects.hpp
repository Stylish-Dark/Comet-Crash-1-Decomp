#pragma once
#include "comet/arena_entity_accumulator.hpp"
#include "comet/arena_counter_emission.hpp"
#include "comet/arena_player_statistics.hpp"
namespace comet::decomp {
enum class ArenaPreparationCompletion { Prepared, MissingEmission, NeedsNumericRecovery, Invalid };
// Original9EC8..A268 after1AD8. Source body-extension address+5D0 becomes the
// relative work-context offset80 on PC; it is never a truncated host pointer.
// 3510 uses an explicit required1E090 emitter and reused descriptor when reached.
// Missing dependencies/numeric boundaries preserve context/stats;
// the already-consumed effects remain in the supplied result for resumption.
// Emitter must not throw/re-enter or mutate context, statistics or descriptor.
ArenaPreparationCompletion complete_arena_entity_preparation(
 ArenaEntityWorkContext&,std::span<const std::uint8_t,0x2900> snapshot,
 const ArenaAccumulatorConsumption&,std::span<std::uint8_t,128> statistics,
 ArenaEmissionDescriptor* reused_descriptor=nullptr,const ArenaCounterEmissionSink& emitter={});
}
