#include "comet/material_textures.hpp"

#include <array>

namespace comet::decomp {
namespace {

constexpr std::array<MtlTextureDirective, 4> kDirectives{{
    {"map_Kd", MaterialTextureSemantic::Diffuse,
               MaterialTextureLoaderKind::Texture2D, 0x38},
    {"map_Ks", MaterialTextureSemantic::Specular,
               MaterialTextureLoaderKind::Texture2D, 0x3C},
    {"bump",   MaterialTextureSemantic::Bump,
               MaterialTextureLoaderKind::Texture2D, 0x40},
    {"cube",   MaterialTextureSemantic::EnvironmentCube,
               MaterialTextureLoaderKind::CubeTexture, 0x44},
}};

}  // namespace

const MtlTextureDirective* classify_mtl_texture_directive(
    std::string_view keyword) {
    for (const auto& directive : kDirectives) {
        if (directive.keyword == keyword) {
            return &directive;
        }
    }
    return nullptr;
}

}  // namespace comet::decomp
