#pragma once

#include <cstdint>
#include <string_view>

namespace comet::decomp {

// Proven model-object fields used by PPU helper 0x00105088.
// The native model type does not need to retain these physical offsets.
inline constexpr std::uint32_t kModelDiffuseFieldOffset = 0x38;
inline constexpr std::uint32_t kModelSpecularFieldOffset = 0x3C;
inline constexpr std::uint32_t kModelBumpFieldOffset = 0x40;
inline constexpr std::uint32_t kModelShaderFieldOffset = 0x48;

enum class DefaultModelShader {
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

struct ModelShaderInputs {
    bool has_diffuse = false;
    bool has_specular = false;
    bool has_bump = false;
    bool shader_already_assigned = false;
    std::uint32_t original_options = 0;
};

// Exact semantic rewrite of PPU 0x00105088.
DefaultModelShader choose_default_model_shader(const ModelShaderInputs& input);

std::string_view default_model_shader_name(DefaultModelShader shader);

}  // namespace comet::decomp
