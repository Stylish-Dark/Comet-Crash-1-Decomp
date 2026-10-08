#pragma once
#include "comet/material_properties.hpp"
#include "comet/model_geometry.hpp"
#include "comet/model_load_parameters.hpp"
#include <filesystem>
#include <istream>
#include <string>
#include <vector>
namespace comet::decomp {
// Native ownership, independent of the legacy PS3 object ABI.
struct NativeVertex {
    ModelPosition position;
    ModelPosition normal;
    float u = 0, v = 0;
};
static_assert(sizeof(NativeVertex) == kLegacyFullVertexStride);
struct NativeMaterial {
    std::string name;
    MaterialProperties properties;
    std::filesystem::path diffuse_texture, specular_texture, bump_texture, cube_texture;
};
struct NativeSubmesh {
    SubmeshDrawRange range;
    std::string material_name;
    NativeMaterial material;
};
struct NativeModel {
    std::vector<NativeVertex> vertices;
    std::vector<std::uint16_t> indices;
    std::vector<NativeSubmesh> submeshes;
    std::vector<std::filesystem::path> material_libraries;
    ModelPosition bounds_min{}, bounds_max{};
    float signed_bounding_radius = 0;
};
struct AssetLoadResult {
    bool ok = true;
    std::size_t line = 0;
    std::string detail;
    explicit operator bool() const { return ok; }
};
// Standard OBJ syntax and convex-fan policy; not yet proven legacy equivalence.
// All parsers preserve output on failure, reject nonfinite numeric input.
AssetLoadResult parse_obj_model(std::istream& input, ModelLoadParameters parameters,
    std::uint32_t options, float vertex_y_offset, NativeModel& output);
AssetLoadResult parse_mtl_materials(std::istream& input, std::vector<NativeMaterial>& output);
} // namespace comet::decomp
