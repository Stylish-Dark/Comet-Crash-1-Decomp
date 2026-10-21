#pragma once
#include "comet/arena_counter_emission.hpp"
#include <vector>
namespace comet::decomp {
// Native storage for source1E090's 128-byte records and source2AD8's FIFO.
// The caller supplies original bootstrap/order and serializes access. FIFO
// capacity must be a multiple of4 (the source's aligned DMA ring), with valid
// available identities and at most65536 records (the stored index is16 bits).
// Publication
// does not initialize game bodies, consume these records, or choose a scheduler.
struct ArenaRequestPublicationQueue {
 std::vector<ArenaEmissionDescriptor> records;
 std::vector<std::uint32_t> free_ring;
 std::uint32_t head=0, available=0;
 // Source2C38 stores all four words of the availability quadword, including
 // these adjacent lanes. Keep their unsigned subtraction, without guessing
 // whether other original callers use them.
 std::array<std::uint32_t,3> availability_lanes{};
};
enum class ArenaRequestPublicationStatus { Published, PoolFull, Invalid };
struct ArenaRequestPublicationResult {
 ArenaRequestPublicationStatus status=ArenaRequestPublicationStatus::Invalid;
 std::uint32_t published=0;
};
// Original1E090..1E47C and2AD8 allocation, using synchronous native record writes
// in place of PS3 DMA. Allocates in batches <=32, preserving a reserve of128.
// Exhaustion retains earlier published batches. Source returns zero even on
// exhaustion; this result exposes bookkeeping without changing that behavior.
// mode!=0 distributes original byte phases across +0E/+2E; mode0 preserves them.
ArenaRequestPublicationResult publish_arena_request(ArenaRequestPublicationQueue&,
 const ArenaCounterEmissionRequest&);
}
