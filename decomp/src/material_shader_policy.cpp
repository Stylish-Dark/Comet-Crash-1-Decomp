#include "comet/material_shader_policy.hpp"

namespace comet::decomp {

DefaultMaterialShader choose_default_material_shader(
    const MaterialShaderInputs& in) {
    if (in.shader_already_assigned) {
        return DefaultMaterialShader::Unchanged;
    }

    const std::uint32_t flags = in.original_options;

    if (!in.has_diffuse) {
        return (flags & 0x4u)
            ? DefaultMaterialShader::LitObjectNoTeamGround
            : DefaultMaterialShader::LitObject;
    }

    if (!in.has_specular) {
        return in.has_bump
            ? DefaultMaterialShader::LitBump
            : DefaultMaterialShader::LitTexture;
    }

    if (!in.has_bump) {
        return (flags & 0x40u)
            ? DefaultMaterialShader::LitTextureSpecGloss
            : DefaultMaterialShader::LitTextureSpec;
    }

    if ((flags & 0x0Cu) == 0x0Cu) {
        return DefaultMaterialShader::LitBumpSpecNoTeamGround;
    }
    if ((flags & 0x244u) == 0x244u) {
        return DefaultMaterialShader::LitBumpSpecGlossGlowNoTeam;
    }
    if ((flags & 0x44u) == 0x44u) {
        return DefaultMaterialShader::LitBumpSpecGlossNoTeam;
    }
    if (flags & 0x4u) {
        return DefaultMaterialShader::LitBumpSpecNoTeam;
    }
    if (flags & 0x40u) {
        return DefaultMaterialShader::Unchanged;
    }
    return DefaultMaterialShader::LitBumpSpec;
}

std::string_view default_material_shader_name(DefaultMaterialShader shader) {
    switch (shader) {
    case DefaultMaterialShader::Unchanged:
        return {};
    case DefaultMaterialShader::LitObject:
        return "lit_object_shader";
    case DefaultMaterialShader::LitObjectNoTeamGround:
        return "lit_object_shader_no_team_ground";
    case DefaultMaterialShader::LitTexture:
        return "lit_texture_shader";
    case DefaultMaterialShader::LitBump:
        return "lit_bump_shader";
    case DefaultMaterialShader::LitTextureSpec:
        return "lit_texture_spec_shader";
    case DefaultMaterialShader::LitTextureSpecGloss:
        return "lit_texture_spec_gloss_shader";
    case DefaultMaterialShader::LitBumpSpec:
        return "lit_bump_spec_shader";
    case DefaultMaterialShader::LitBumpSpecNoTeam:
        return "lit_bump_spec_shader_no_team";
    case DefaultMaterialShader::LitBumpSpecNoTeamGround:
        return "lit_bump_spec_shader_no_team_ground";
    case DefaultMaterialShader::LitBumpSpecGlossNoTeam:
        return "lit_bump_spec_gloss_shader_no_team";
    case DefaultMaterialShader::LitBumpSpecGlossGlowNoTeam:
        return "lit_bump_spec_gloss_glow_shader_no_team";
    }
    return {};
}

}  // namespace comet::decomp
