#pragma once
#include "comet/arena_entity_preparation.hpp"
#include <functional>
namespace comet::decomp {
using ArenaAccumulatorRecord=std::array<std::uint8_t,128>;
enum class ArenaAccumulatorWrite { Committed, Retry, Unavailable };
struct ArenaAccumulatorTransport {
 // Platform reservation boundary: load128 bytes from the supplied source EA;
 // conditional write must preserve the reservation and return Retry on loss.
 std::function<std::optional<ArenaAccumulatorRecord>(std::uint32_t)> read_reserved;
 std::function<ArenaAccumulatorWrite(std::uint32_t,const ArenaAccumulatorRecord&)> write_conditional;
};
enum class ArenaAccumulatorStatus { Cached, Consumed, MissingTransport, Invalid };
struct ArenaAccumulatorConsumption {
 ArenaAccumulatorStatus status=ArenaAccumulatorStatus::Invalid;
 std::array<std::uint8_t,16> health_10{},impulse_40{};
 std::uint32_t request_30=0,attempts=0;
};
// Original1AD8..2054: pending gate, fresh reservation snapshot, original clear
// mask, retry after failed conditional write, and restoration of consumed data
// in the local record. Complete platform transport must be supplied if pending.
// Supported arithmetic is normal/zero finite health and impulse XYZ.
ArenaAccumulatorConsumption consume_arena_entity_accumulator(
 ArenaAccumulatorRecord&,const ArenaAccumulatorTransport&);
// The9A70 caller supplies context+300 to1AD8. Does not run9EC8 continuation.
ArenaAccumulatorConsumption consume_arena_prepared_accumulator(
 ArenaEntityWorkContext&,const ArenaAccumulatorTransport&);
}

namespace comet::decomp {
enum class ArenaCommandClaimStatus { Claimed, Occupied, MissingTransport, Invalid };
struct ArenaCommandClaimResult {
 ArenaCommandClaimStatus status=ArenaCommandClaimStatus::Invalid;
 std::uint32_t attempts=0;
};
// Original14F8: reserve the target's128-byte accumulator, claim empty word60,
// preserve all other bytes, and retry fresh after failed PUTLLC. Occupied slots
// still require a successful conditional write before returning Occupied.
ArenaCommandClaimResult claim_arena_entity_command(ArenaAccumulatorRecord& scratch,
 std::uint32_t accumulator_base,std::int16_t target,
 const std::array<std::uint8_t,16>& payload,const ArenaAccumulatorTransport&);
}
