#pragma once
#include "comet/arena_entity_accumulator.hpp"
#include "comet/arena_particle_emission.hpp"
#include "comet/arena_audio_event.hpp"
#include <functional>
namespace comet::decomp {
struct ArenaPlayerDeathScratch {
 // Original reused640/680/630 stack configuration quadwords.
 std::array<std::array<std::uint8_t,16>,3> configurations{};
 ArenaAccumulatorRecord statistics{};
};
struct ArenaPlayerDeathEffects {
 std::array<ArenaParticleEmission,3> particles{};
 bool reset=false,without_base=false;
};
//12288 pure writes/configuration, with12780 reset selected by2770/28E0.
// Entry is after10290 has set590 for a depleted BC0 craft. Statistics-derived
// D4 is applied during publication, after BF58 returns its committed record.
// Rejects without mutation. Does not publish or advance RNG.
bool prepare_arena_player_death(ArenaEntityWorkContext&,
 std::span<const std::uint8_t,0x2900>,ArenaPlayerDeathScratch&,ArenaPlayerDeathEffects&);
// Source sound7 ->three1E880 calls (1F030 before the third) ->BF58/BB30.
// Complete transports must not throw/reenter/inspect controller inputs. Host
// failure after an effect is fatal, never a resumable controller rejection.
// Reservations load a fresh128-byte owner record after each failed PUTLLC.
void publish_arena_player_death(ArenaEntityWorkContext&,
 std::span<const std::uint8_t,0x2900>,ArenaPlayerDeathScratch&,const ArenaPlayerDeathEffects&,
 ArenaRandom&,const std::function<void(const ArenaParticleEmission&,ArenaRandom&)>&,
 const ArenaAudioEventSink&,const ArenaAccumulatorTransport&,ArenaGridPosition original_job_cell);
}
