#pragma once
#include "comet/arena_ground_contact.hpp"
#include "comet/arena_routes.hpp"
namespace comet::decomp {
struct ArenaGroundTerrainGeometryQuery {
 ArenaGridPosition cell{};
 std::uint8_t width=24,height=24,visibility_bit=0;
 bool world_only=false;
};
struct ArenaGroundTerrainGeometry {
 std::array<float,4> limits{}; // X-min, X-max, Z-min, Z-max
 std::array<std::array<float,4>,4> corners{}; // NW, SW, NE, SE; zero placeholders retained
};
// Original 20 local rectangle and diagonal corner-point selection.
std::optional<ArenaGroundTerrainGeometry> prepare_arena_ground_terrain(
 const ArenaGroundTerrainGeometryQuery &,const ArenaRoutingGrid &);
struct ArenaGroundPointQuery {
 std::array<float,4> position{},velocity{},reference_position{},point{},accumulated_impulse{};
 float radius=.1f;
 std::uint8_t restitution_percent=100;
 bool movable=true;
};
// Motion portion of 7970: test the reference position, correct current position.
std::optional<ArenaGroundBoundsResult> resolve_arena_ground_point(const ArenaGroundPointQuery &);
struct ArenaGroundTerrainQuery {
 ArenaGroundTerrainGeometryQuery geometry;
 ArenaGroundBoundsQuery bounds;
 std::array<float,4> reference_position{};
};
// Six-plane response then four corner points in original order.
std::optional<ArenaGroundBoundsResult> contain_arena_ground_terrain(
 const ArenaGroundTerrainQuery &,const ArenaRoutingGrid &);
}
