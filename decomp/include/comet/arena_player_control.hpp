#pragma once
#include "comet/arena_counter_emission.hpp"
#include <functional>
namespace comet::decomp {
struct ArenaPlayerControlScratch {
 ArenaEntitySlot pending_entity{};
 ArenaEmissionDescriptor owner_effect{};
};
// Source2058 allocation/spatial submission, then2198 body publication. A failed
// allocation returns-1:3238 retains flag8000 and the controller still proceeds.
// Callbacks own the actual original queue/storage; neither success nor fullness
// is inferred. They must not throw, reenter, or inspect/mutate controller inputs.
struct ArenaPlayerPendingTransport {
 std::function<std::int32_t(const ArenaEntitySlot&)> reserve;
 std::function<void(const ArenaEntitySlot&)> publish;
};
enum class ArenaPlayerControlStatus {
 Advanced, Invalid, NeedsPendingQueue, NeedsPeerContact, NeedsAlternateControl,
 NeedsAttachment, NeedsTargetFacing, NeedsCarry, NeedsDeath, NeedsBanking,
 NeedsOwnerEmission, NeedsContactEffects, NeedsNumericRecovery, NeedsFinalization
};
struct ArenaPlayerControlResult {
 ArenaPlayerControlStatus status=ArenaPlayerControlStatus::Invalid;
 std::uint32_t blocked_address=0;
};
// Original10290 entry through ordinary-mode0 return113A4, including3238 request
// construction,7DD0 bounds, no-target7E40, movement-facing, owner timer/request
// and body/controller commits. Other source branches return a named boundary.
// Uses the completed9A70 context, relative extension reference80 and original
// snapshot. Supports finite normal/zero values in the documented math domain.
// Rejection preserves context/scratch and issues no external callbacks. Callback
// contracts apply equally to emitter. Finalization and scheduling are separate.
ArenaPlayerControlResult advance_arena_player_control(ArenaEntityWorkContext&,
 std::span<const std::uint8_t,0x2900> snapshot,float elapsed,
 ArenaPlayerControlScratch* scratch=nullptr,
 const ArenaPlayerPendingTransport& pending={},
 const ArenaCounterEmissionSink& emitter={});
}
