#pragma once
#include "comet/arena_entity_storage.hpp"
#include <stdexcept>
namespace comet::decomp {
struct ArenaAreaEffectQuery {
 std::array<float,4> center{};
 ArenaGridPosition cell{};
 ArenaPackedCellEntry entry{};
 std::uint32_t stage_mask=0,owner_mask=0;
 float radius=1,damage=0,impulse_strength=0;
 bool exclude_marker_two=false;
};
struct ArenaAreaEffectHit {
 std::uint16_t index=0;
 float health_delta=0;
 std::array<float,4> impulse{};
};
// FE5D8 8AF0: broad phase over each caller-selected neighboring cell.
// Center Y is ignored here. Invalid native numeric inputs throw before work.
bool arena_area_effect_cell_overlaps(const std::array<float,4>& center,
                                    ArenaGridPosition cell,float radius);
// FE5D8 per-entry eligibility/geometry/falloff, with original exponent fixed
// to 3 as in both recovered callers. Uses packed entry coordinates/radius.
// Returns a signed health delta and radial impulse without mutating storage.
// Caller owns ordered 3x3 traversal, source attribution and accumulation.
// Native bounds: |center XYZ|<=64, radius in [0.0001,64], cell in 24x24.
// Invalid/nonfinite/subnormal native numeric inputs throw invalid_argument.
std::optional<ArenaAreaEffectHit> evaluate_arena_area_effect_cubic(const ArenaAreaEffectQuery&);
}
