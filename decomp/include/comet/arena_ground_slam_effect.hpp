#pragma once
#include "comet/arena_entity_storage.hpp"
namespace comet::decomp {
// Native ordered 3x3 gather for the recovered kind-44 impact emission.
// Only its health lane is accumulated; original effects/attribution are separate.
// Invalid input leaves pending health unchanged; valid non-trigger returns empty.
std::optional<std::vector<std::uint16_t>> accumulate_arena_ground_slam(
 const ArenaEntityStorage &,std::uint16_t source,std::uint32_t owner_mask,
 const std::array<float,4> &contact_impulse,std::span<float> pending_health);
}
