#pragma once

#include <cstdint>
#include <string_view>

namespace comet::decomp {

// PPU helper 0x00105088 receives the material subobject at submesh+0x10,
// not the root model object. These offsets are relative to that material
// subobject. Absolute offsets within the 0x78-byte submesh record are +0x48,
// +0x4C, +0x50 and +0x58 respectively.
inline constexpr std::uint32_t kMaterialDiffuseFieldOffset = 0x38;
inline constexpr std::uint32_t kMaterialSpecularFieldOffset = 0x3C;
inline constexpr std::uint32_t kMaterialBumpFieldOffset = 0x40;
inline constexpr std::uint32_t kMaterialShaderFieldOffset = 0x48;

enum class DefaultMaterialShader {
    Unchanged,
    LitObject,
    LitObjectNoTeamGround,
    LitTexture,
    LitBump,
    LitTextureSpec,
    LitTextureSpecGloss,
    LitBumpSpec,
    LitBumpSpecNoTeam,
    LitBumpSpecNoTeamGround,
    LitBumpSpecGlossNoTeam,
    LitBumpSpecGlossGlowNoTeam,
};

struct MaterialShaderInputs {
    bool has_diffuse = false;
    bool has_specular = false;
    bool has_bump = false;
    bool shader_already_assigned = false;
    std::uint32_t original_options = 0;
};

// Exact semantic rewrite of PPU 0x00105088..0x00105307.
DefaultMaterialShader choose_default_material_shader(
    const MaterialShaderInputs& input);

std::string_view default_material_shader_name(DefaultMaterialShader shader);

}  // namespace comet::decomp
