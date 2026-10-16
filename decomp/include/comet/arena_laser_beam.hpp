#pragma once
#include "comet/arena_entity_storage.hpp"
namespace comet::decomp {
enum class ArenaLaserBeamStatus { Active, Retire, Unsupported, Invalid };
struct ArenaLaserBeamStep {
 ArenaLaserBeamStatus status=ArenaLaserBeamStatus::Invalid;
 std::uint16_t target_index=0xffff;
 float health_delta=0;
 // Apply this second, separately: ordered accumulation matches the two messages.
 float extra_health_delta=0;
 unsigned parent_command_increment=0;
};
// Kind-5 update19748 after9A70 preparation, for a kind-22 parent. Parent/target
// are snapshots selected by beam+1E/+1C. Null target follows original idle path.
// Updates the embedded80..FF five-point display state and core RNG draw order.
// The visual allocation result is an explicit boundary. With no particle
// backend, false models allocation failure. Success consumes five additional
// shared RNG draws on a hit with elapsed > floatbits 3A83126E.
// Native retirement returns Retire; caller owns pool/cell cleanup and damage.
// Admitted inputs: elapsed (0,.25], XYZ positions within +/-64, velocities
// within +/-1024, scalar damage/timers within +/-1e6, ordinal below eight.
// Nonfinite/subnormal scalar operands are invalid; display W lanes are opaque
// metadata carried by the original SIMD arithmetic.
// Invalid input rolls back both slot and RNG. No callbacks or world writes.
ArenaLaserBeamStep advance_arena_laser_beam(ArenaEntitySlot &,const ArenaEntitySlot &parent,
                                          const ArenaEntitySlot *target,float elapsed,ArenaRandom &,bool hit_effect_available=false);
}
