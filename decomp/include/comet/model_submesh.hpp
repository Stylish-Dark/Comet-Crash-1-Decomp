#pragma once

#include "comet/material_programs.hpp"
#include "comet/material_properties.hpp"
#include "comet/model_geometry.hpp"

#include <string>

namespace comet::decomp {

// Native resource names/paths recovered from one MTL material. Empty strings
// mean that the corresponding directive/resource is absent.
struct MaterialTexturePaths {
    std::string diffuse;
    std::string specular;
    std::string bump;
    std::string environment_cube;
};

// Semantic replacement for the legacy material subobject at submesh+0x10.
// No fixed PS3 ABI, raw resource pointers, Cg handles, or SPU state leaks into
// this type.
struct ModelMaterial {
    MaterialProperties properties{};
    MaterialTexturePaths textures{};
    std::string shader_base;
    MaterialProgramPaths legacy_program_paths{};
};

// Semantic replacement for one legacy 0x78-byte submesh record.
struct ModelSubmesh {
    SubmeshDrawRange draw_range{};
    ModelMaterial material{};

    // Proven legacy capability bit at submesh+0x64. A native renderer can map
    // this to ordinary instancing instead of reproducing SPU-expanded streams.
    bool legacy_batched_path_available = false;
};

}  // namespace comet::decomp
