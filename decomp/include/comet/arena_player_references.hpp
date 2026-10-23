#pragma once
#include "comet/arena_entity_accumulator.hpp"
namespace comet::decomp {
// B500 fetches256 bytes at600. The existing680-byte work slice retains its
// first128 bytes; own the remaining128 separately instead of overrunning it.
struct ArenaPlayerReferenceCache {
 std::array<std::uint8_t,128> attachment_tail{};
};
// SourceB500 ->3108, then21F8: cache the current body's accumulator, optional
// peers1C/1E and attached entityB8, and store the original job tag at59C.
// The bank is the original snapshot+27D0 / source26B90 read bank; these are the
// same pointer, set at510..58C. Access is caller-serialized. Sentinels preserve
// reused bytes. Native out-of-bounds/signed-negative references reject before
// writes. Does not consume accumulators, run9A70 or choose a job scheduler.
bool fetch_arena_player_references(ArenaEntityWorkContext&,ArenaPlayerReferenceCache&,
 std::span<const ArenaEntitySlot> read_bank,
 std::span<const ArenaAccumulatorRecord> accumulator_records,
 std::uint32_t original_job_tag);
}
