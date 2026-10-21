#pragma once
#include "comet/arena_entity_storage.hpp"
namespace comet::decomp {
// Original FE5D8 job context, including the body at +0. Unwritten bytes belong
// to the caller; this is ordinary game data, not an SPU execution context.
using ArenaEntityWorkContext = std::array<std::uint8_t,0x680>;
using ArenaPreparationSnapshot = std::array<std::uint8_t,0x2900>;
// Original9A70..9EC0 state staging, stopping BEFORE1AD8 consumes accumulators.
// Snapshot is the original job's root image: tile16-byte records, +2710 vector,
// +2720 player halfwords, +28D8/+28DC extents. No snapshot policy is invented.
// Supports finite normal/zero XYZ/quaternion within +/-1e6, owner0..3,
// extents1..24, normal/zero intermediate arithmetic. Invalid inputs
// preserve context. Success does not mean the rest of9A70/controller ran.
bool stage_arena_entity_preparation(ArenaEntityWorkContext&,
 std::span<const std::uint8_t,0x2900> snapshot);
}
