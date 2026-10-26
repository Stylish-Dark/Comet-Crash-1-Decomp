#pragma once
#include "comet/arena_counter_emission.hpp"
#include "comet/arena_particle_emission.hpp"
#include <functional>
namespace comet::decomp {
struct ArenaPlayerControlScratch {
 ArenaEntitySlot pending_entity{};
 ArenaEmissionDescriptor owner_effect{};
 ArenaEmissionDescriptor action_effect{};
 // Four reused original interaction proxy records (far/near, left/right).
 std::array<ArenaEntitySlot,4> interaction_proxies{};
};
// Source2058 allocation/spatial submission, then2198 body publication. A failed
// allocation returns-1:3238 retains flag8000 and the controller still proceeds.
// Callbacks own the actual original queue/storage; neither success nor fullness
// is inferred. They must not throw, reenter, or inspect/mutate controller inputs.
struct ArenaPlayerPendingTransport {
 std::function<std::int32_t(const ArenaEntitySlot&)> reserve;
 std::function<void(const ArenaEntitySlot&)> publish;
};
// Original particle-pool ownership remains explicit. Publisher executes1E880
// with the supplied RNG, including no draws on a failed pool batch. Same
// no-throw/no-reentry/input-observation contract as the pending callbacks.
struct ArenaPlayerParticleTransport {
 ArenaRandom* random=nullptr;
 std::function<void(const ArenaParticleEmission&,ArenaRandom&)> publish;
};
enum class ArenaPlayerControlStatus {
 Advanced, Invalid, NeedsPendingQueue, NeedsPeerContact, NeedsAlternateControl,
 NeedsAttachment, NeedsTargetFacing, NeedsCarry, NeedsDeath, NeedsBanking,
 NeedsOwnerEmission, NeedsActionEmission, NeedsContactEffects, NeedsNumericRecovery, NeedsFinalization
};
struct ArenaPlayerControlResult {
 ArenaPlayerControlStatus status=ArenaPlayerControlStatus::Invalid;
 std::uint32_t blocked_address=0;
};
// Original10290 entry through supported ordinary/banking returns, including3238 request
// construction,7DD0 bounds, no-target7E40, movement/target-facing, ordinary
// action edge/visual requests/candidate selection, owner timer/request
// and body/controller commits. Also recovers11CA8 grid approach,119BC positive
// attachment countdown and10B60 arm banking/four180A0 proxy lifecycles. Missing
// death/attachment/peer-contact continuations return a named boundary.11A80
// full expansion emits four original particle calls through the supplied pool/RNG
// transport. Hard7DD0 craft impacts have source particle count0 and can advance
// without a particle transport. Proxy storage
// order is far-left, near-left, far-right, near-right; linksC8/CC/CA/CE.
// Uses the completed9A70 context, relative extension reference80 and original
// snapshot. Supports finite normal/zero values in the documented math domain.
// Rejection preserves context/scratch and issues no external callbacks. Callback
// contracts apply equally to emitter. Finalization and scheduling are separate.
ArenaPlayerControlResult advance_arena_player_control(ArenaEntityWorkContext&,
 std::span<const std::uint8_t,0x2900> snapshot,float elapsed,
 ArenaPlayerControlScratch* scratch=nullptr,
 const ArenaPlayerPendingTransport& pending={},
 const ArenaCounterEmissionSink& emitter={},
 const ArenaPlayerParticleTransport& particles={});
}
