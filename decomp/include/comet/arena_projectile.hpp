#pragma once
#include "comet/arena_entity_storage.hpp"
namespace comet::decomp {
enum class ArenaProjectileStatus { Active, Retire, Unsupported, Invalid };
// Prepared 5CE0 segment-query result. Type 6 names a damage-message recipient;
// other nonzero types collide with world geometry. Caller owns collision queries.
struct ArenaProjectileContact {
 bool hit=false;
 unsigned type=0;
 std::uint16_t target_index=0xffff;
 // Optional 7A50 work-state correction, before acceleration/integration.
 std::optional<std::array<float,4>> bounds_position, bounds_velocity;
 // Optional 7E40 corrected work position, after predicted integration.
 std::optional<std::array<float,4>> corrected_position;
};
struct ArenaProjectileStep {
 ArenaProjectileStatus status=ArenaProjectileStatus::Invalid;
 bool impact_message=false;
 std::uint16_t target_index=0xffff;
 float health_delta=0;
 std::array<float,4> impulse{};
};
// Kind-3 AE58 and kind-15 AA60 after generic9A70 preparation, with no visual-pool allocation.
// Visual allocation failure consumes no RNG. Kind 3 adds acceleration when Y>1,
// updates its direction quaternion, and increments +80. A type-6 hit emits negative zero
// health and velocity*+58 impulse. Caller applies messages and owns retirement.
// The 7A50 bounds result is supplied separately. Active finalization copies XYZ
// only; retirement clears flag bit2 without copying the predicted position.
// Admits dt(0,.25], XYZ +/-64, velocity +/-1024, impulse scale +/-1e6;
// nonfinite/subnormal arithmetic and out-of-range contact types reject atomically.
// W metadata and all other opaque slot bytes are preserved.
ArenaProjectileStep advance_arena_projectile(ArenaEntitySlot &,float elapsed,
 const ArenaProjectileContact &contact={},bool outside_bounds=false,
 const std::array<float,4> &acceleration={});
}
