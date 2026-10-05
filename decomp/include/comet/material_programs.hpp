#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace comet::decomp {

// Program/resource fields inside the legacy material subobject at submesh+0x10.
// PPU helper 0x00101960 constructs all three filenames from one recovered
// shader base name and stores the loaded resources at these offsets.
struct LegacyMaterialProgramOffsets {
    static constexpr std::uint32_t standard_vertex_program = 0x48;
    static constexpr std::uint32_t batched_vertex_program = 0x4C;
    static constexpr std::uint32_t fragment_program = 0x50;
    static constexpr std::uint32_t batch_enabled = 0x54;
};

inline constexpr std::string_view kLegacyVertexProgramSuffix = ".vpo";
inline constexpr std::string_view kLegacyBatchedVertexProgramSuffix = "_spu.vpo";
inline constexpr std::string_view kLegacyFragmentProgramSuffix = ".fpo";

struct MaterialProgramPaths {
    std::string standard_vertex;
    std::string batched_vertex;
    std::string fragment;
};

// Exact filename policy recovered from PPU 0x00101960.
// A shader base such as "lit_texture_shader" becomes:
//   lit_texture_shader.vpo
//   lit_texture_shader_spu.vpo
//   lit_texture_shader.fpo
MaterialProgramPaths material_program_paths(std::string_view shader_base);

}  // namespace comet::decomp
