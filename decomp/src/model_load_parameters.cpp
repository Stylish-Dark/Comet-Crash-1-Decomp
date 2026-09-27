#include "comet/model_load_parameters.hpp"

#include <cmath>

namespace comet::decomp {

ModelPosition apply_geometry_scale(
    ModelPosition position,
    float geometry_scale) {
    position.x *= geometry_scale;
    position.y *= geometry_scale;
    position.z *= geometry_scale;
    return position;
}

ModelPosition transform_imported_position(
    ModelPosition position,
    const ModelLoadParameters& parameters) {
    position = apply_geometry_scale(position, parameters.geometry_scale);

    // Exact original branch: rlwinm tests option mask 0x20, bf 6 enters
    // 0x0010AB3C when the selected condition is false. In that path +0x28 is
    // added to the scaled Y component.
    if ((parameters.original_options & 0x20u) == 0u) {
        position.y += parameters.geometry_y_offset;
    }

    return position;
}

float compute_signed_bounding_radius(
    std::span<const ModelPosition> scaled_positions,
    float signed_radius_scale) {
    const float magnitude_scale = std::fabs(signed_radius_scale);
    float radius = 0.0f;

    for (const auto& p : scaled_positions) {
        const float distance =
            std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
        const float candidate = magnitude_scale * distance;
        if (candidate > radius) {
            radius = candidate;
        }
    }

    return signed_radius_scale < 0.0f ? -radius : radius;
}

}  // namespace comet::decomp
