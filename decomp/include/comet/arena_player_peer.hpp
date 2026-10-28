#pragma once
#include "comet/arena_entity_accumulator.hpp"
#include "comet/arena_particle_emission.hpp"
namespace comet::decomp {
struct ArenaPlayerPeerResult {
 bool advanced=false, overlapping=false, outgoing_impulse=false;
 std::uint16_t peer_index=65535;
 std::array<float,4> impulse{};
 std::optional<ArenaParticleEmission> particles;
};
// Original7E40 single-peer player call (11618), including7510 response.
// Uses the actual fetched peer at100 and source target halfword1C. Source
// stage5 hostile kind45 capture remains an explicit unsupported boundary.
// Stages context/RNG; failure leaves both intact. Returned effects are ordered
// original16F8 then1E880 calls and must be executed by the caller.
ArenaPlayerPeerResult prepare_arena_player_peer_contact(ArenaEntityWorkContext&,
 std::span<const std::uint8_t,0x2900> snapshot,ArenaRandom&);
// Original16F8 fixed contact arguments: zero health/request/counters, supplied
// impulse, flags0. Fresh reservation is loaded after each failed write.
ArenaAccumulatorStatus submit_arena_peer_impulse(ArenaAccumulatorRecord& scratch,
 std::uint32_t source_ea,const std::array<float,4>&,const ArenaAccumulatorTransport&);
}
