#pragma once

#include <span>

namespace comet::decomp {

struct ModelPosition {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct ModelLoadParameters {
    // Original f1. PPU 0x0010751C recovers it unchanged from the entry value,
    // and the three OBJ vertex components are multiplied by it before storage.
    float geometry_scale = 1.0f;

    // Original f2. The loader takes abs(f2), multiplies it by each scaled
    // position's radial distance, keeps the maximum at model+0x2C, then restores
    // the sign of f2 at finalization.
    float signed_radius_scale = 0.0f;
};

ModelPosition apply_geometry_scale(
    ModelPosition position,
    float geometry_scale);

// Semantic replacement for the model+0x2C accumulation/finalization performed
// by PPU 0x00107684..0x001076D4 / 0x00108460..0x00108464 / 0x00105D24..0x00105D54.
float compute_signed_bounding_radius(
    std::span<const ModelPosition> scaled_positions,
    float signed_radius_scale);

}  // namespace comet::decomp
