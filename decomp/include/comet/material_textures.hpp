#pragma once

#include <cstdint>
#include <string_view>

namespace comet::decomp {

// Material subobject begins at submesh+0x10 in the legacy 0x78-byte submesh.
// Offsets below are relative to that material subobject.
struct LegacyMaterialTextureOffsets {
    static constexpr std::uint32_t diffuse = 0x38;
    static constexpr std::uint32_t specular = 0x3C;
    static constexpr std::uint32_t bump = 0x40;
    static constexpr std::uint32_t environment_cube = 0x44;
    static constexpr std::uint32_t shader = 0x48;
};

enum class MaterialTextureSemantic {
    Diffuse,
    Specular,
    Bump,
    EnvironmentCube,
};

enum class MaterialTextureLoaderKind {
    Texture2D,
    CubeTexture,
};

struct MtlTextureDirective {
    std::string_view keyword;
    MaterialTextureSemantic semantic{};
    MaterialTextureLoaderKind loader_kind{};
    std::uint32_t legacy_material_offset = 0;
};

// Exact directive map recovered from PPU 0x0010A678..0x0010B36C.
const MtlTextureDirective* classify_mtl_texture_directive(
    std::string_view keyword);

}  // namespace comet::decomp
