#pragma once
#include "comet/arena_entity_storage.hpp"
namespace comet::decomp {
struct ArenaTeslaSourceResult {
  bool spawn_beam = false;
  std::optional<std::uint16_t> target;
};
// Bounded one-beam kind-22 source, SPU 18908. Native beam existence supplies
// the original asynchronous active count; successful insertion owns that count.
// Only +90, +B4, +B8, +C4 and +C8 are updated; visual phase/orientation is separate.
std::optional<ArenaTeslaSourceResult> advance_arena_tesla_source(
    ArenaEntitySlot &tower, float elapsed, std::uint32_t parent_commands,
    std::optional<std::uint16_t> selected_target, bool beam_exists);
// SPU 18F28..190B0, ordinal zero. Preserves bytes the original leaves untouched.
// Pool insertion supplies the beam index; target remains FFFF until acquisition.
bool initialize_arena_tesla_beam(ArenaEntitySlot &beam,
                                const ArenaEntitySlot &tower);
// SPU 4078..4218 with source stage mask 62. Distance is between integer cells;
// route is the candidate cell's directed candidate-owner -> source-owner byte.
// Gathering bounds/order are the caller's responsibility; higher score wins,
// and exact ties retain the first encountered candidate.
std::optional<float> arena_tesla_target_score(
    const ArenaPackedCellEntry &, float cell_distance,
    std::uint8_t directed_route, std::uint16_t source_team,
    std::uint16_t candidate_team);
// General 4428 distance leaf; local cached 3x3 uses fixed 1 or sqrt(2) weights.
// Rejects zero (excluded from the original broad scan) and offsets beyond 23.
std::optional<float> arena_tesla_cell_distance(int dx, int dz);
} // namespace comet::decomp
