#pragma once
#include "comet/arena_entity_preparation.hpp"
#include "comet/arena_entity_accumulator.hpp"
#include "comet/arena_entity_finalization.hpp"
#include "comet/arena_entity_spatial.hpp"
#include "comet/arena_audio_event.hpp"
namespace comet::decomp {
//12CD0 uses A00/B00, beyond the680-byte craft work slice.
using ArenaGroundCarryContext=std::array<std::uint8_t,0xc00>;
enum class ArenaGroundCarryStatus {
 Advanced, Invalid, NeedsParent, NeedsCaller, NeedsNumericRecovery, NeedsCommands,
 NeedsFinalization, CellFull, NeedsAudio
};
// Original12CD0/3108 references.FFFF retains reused cache bytes; signed-negative
// IDs outside the supported native pool reject before any write.
bool fetch_arena_ground_carry_references(ArenaGroundCarryContext&,
 std::span<const ArenaEntitySlot>,std::span<const ArenaAccumulatorRecord>,std::uint32_t tag);
//14538 linked formation correction, before14188 contact/integration/bounds.
// Does not claim the remaining14060 caller. Reads actual cached A00 parent.
ArenaGroundCarryStatus prepare_arena_ground_carry_position(ArenaGroundCarryContext&,
 std::span<const std::uint8_t,0x2900> snapshot,float elapsed);
//158B8's linked path, including17EE8 parent-loss clear of B4/B6 and common
// request-free tail. Requires completed14060 motion at480/4A0, not guessed state.
ArenaGroundCarryStatus advance_arena_ground_carry_dynamics(ArenaGroundCarryContext&,
 std::span<const std::uint8_t,0x2900> snapshot,float elapsed);
// Owned adapter for already-prepared motion ->linked158B8 ->alive6578/6260 ->banks.
// No preview steering, elapsed scheduling, link creation or unknown6F40 event
// handler is fabricated. Source frame/cache/statistics are explicit inputs.
ArenaGroundCarryStatus commit_prepared_arena_ground_carry(ArenaEntityStorage&,
 std::uint16_t index,ArenaGroundCarryContext&,std::span<std::uint8_t,128> statistics,
 std::span<const std::uint8_t,0x2900> snapshot,float elapsed,std::uint32_t frame,
 ArenaGridPosition original_job_cell,
 const ArenaHealthDisplayTransport& display={});
// Original15FF0 command continuation after158B8 motion/contact output.
// Commands2/3 implement carrier link/ack; other opcodes enter the BC/C4 tail.
// Claimed writes B4 parent and B2 slot from364/368; Occupied still advances
// opcode360 to3 but preserves links. Missing transport preserves all inputs.
// Handles the request-free16060 timer tail; no full ordinary158B8 is invented.
ArenaGroundCarryStatus complete_arena_ground_carry_command(ArenaGroundCarryContext&,
 std::span<const std::uint8_t,0x2900> snapshot,float elapsed,
 ArenaAccumulatorRecord& scratch,const ArenaAccumulatorTransport&,
 ArenaCommandClaimResult* claim=nullptr,ArenaGridPosition original_job_cell={},
 const ArenaAudioEventSink& audio={});
// Pure source6F40 alive/no-base ->6578/6260 commit boundary. Also resumes a
// completed command without replaying its externally published claim.
ArenaGroundCarryStatus finalize_prepared_arena_ground_carry(ArenaEntityStorage&,
 std::uint16_t index,ArenaGroundCarryContext&,std::span<std::uint8_t,128> statistics,
 std::span<const std::uint8_t,0x2900> snapshot,std::uint32_t frame,
 ArenaGridPosition original_job_cell,const ArenaHealthDisplayTransport& display={});

//15FA0/16878 contact-driven ground-carrier request. Source flags1000/peer2000,
// strict gap<.4 and no pending200000 gate it. Claims command2 on the actual peer;
// success consumes one B0 slot and waits for command3 acknowledgement.
ArenaGroundCarryStatus request_arena_ground_carry_pickup(ArenaGroundCarryContext&,
 std::span<const std::uint8_t,0x2900> snapshot,ArenaAccumulatorRecord& scratch,
 const ArenaAccumulatorTransport&,ArenaCommandClaimResult* claim=nullptr);

}
