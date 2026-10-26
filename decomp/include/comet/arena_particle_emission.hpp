#pragma once
#include "comet/arena_request_publication.hpp"
namespace comet::decomp {
// Original1E880 arguments. Configuration is the original packed quadword:
// float0 fade limit, float4 acceleration, byte8 first model, byte9 model
// count, byte10 directed-circle flag, byte11 flag7F, byte12 scalar78.
struct ArenaParticleEmission {
 std::array<float,4> position{},velocity{};
 float radius_min=0,radius_max=0,speed=0,lifetime_min=0,lifetime_max=0;
 std::uint32_t count=0;
 std::array<std::uint8_t,16> configuration{};
};
struct ArenaParticleEmissionScratch {
 // Original32 stack records are reused across batches/calls, not zeroed.
 std::array<ArenaEmissionDescriptor,32> records{};
};
// Original1E880/1E480,2AD8 FIFO reservation and128-byte DMA publication.
// Caller supplies queue/bootstrap, reused scratch and original RNG ownership.
// Exhaustion retains prior batches, consumes no draws for the failed batch.
// Conservative arithmetic: finite normal/zero XYZ/scalars within1e6; finite
// W lanes may retain packed-body subnormal bits.
ArenaRequestPublicationResult publish_arena_particles(ArenaRequestPublicationQueue&,
 const ArenaParticleEmission&,ArenaRandom&,ArenaParticleEmissionScratch&);
}
