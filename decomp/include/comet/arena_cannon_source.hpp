#pragma once
#include "comet/arena_entity_storage.hpp"
namespace comet::decomp {
struct ArenaCannonTarget {
 std::uint16_t index=0xffff;
 std::array<float,4> position{}, velocity{};
 std::uint8_t height_byte=0;
};
struct ArenaCannonSourceQuery {
 float elapsed=0;
 // Caller supplies admitted cached target snapshots (original stage mask 126).
 // Ground-only admission and the original visibility observer are separate.
 // Gather supplies the replacement only when requested.
 std::optional<ArenaCannonTarget> target;
 std::optional<std::uint16_t> gathered_target;
 std::array<float,4> owner_base_center{};
};
struct ArenaCannonFireIntent {
 std::array<float,4> position{}, target_position{}, orientation{};
};
struct ArenaCannonSourceResult {
 bool gather_target=false;
 std::optional<ArenaCannonFireIntent> fire;
};
// Kind21 wrapper1D1B0 ->1CF88 ->1B818. Prior terrain observer88F0
// (B0/B4 scan mask and D0 iterator) is a separate caller boundary.
// Unsupported inputs leave every source byte untouched. Pool allocation is
// caller-owned; the original fire gate resets its timer before allocation.
// Zero idle direction uses a finite analytic limit (no turn).
std::optional<ArenaCannonSourceResult> advance_arena_cannon_source(
 ArenaEntitySlot&,const ArenaCannonSourceQuery&);
// Kind3/model27 constructor1D028..1D17C. Preserves all unwritten bytes.
bool initialize_arena_cannon_projectile(ArenaEntitySlot&,const ArenaEntitySlot&,
 const ArenaCannonFireIntent&);
} // namespace comet::decomp
