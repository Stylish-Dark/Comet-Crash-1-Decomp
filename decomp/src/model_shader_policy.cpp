#include "comet/model_shader_policy.hpp"

namespace comet::decomp {

DefaultModelShader choose_default_model_shader(const ModelShaderInputs& in) {
    // Original +0x48 short-circuit: never overwrite a shader already assigned
    // by an earlier/custom path.
    if (in.shader_already_assigned) {
        return DefaultModelShader::Unchanged;
    }

    const std::uint32_t flags = in.original_options;

    if (!in.has_diffuse) {
        return (flags & 0x4u)
            ? DefaultModelShader::LitObjectNoTeamGround
            : DefaultModelShader::LitObject;
    }

    if (!in.has_specular) {
        return in.has_bump
            ? DefaultModelShader::LitBump
            : DefaultModelShader::LitTexture;
    }

    if (!in.has_bump) {
        return (flags & 0x40u)
            ? DefaultModelShader::LitTextureSpecGloss
            : DefaultModelShader::LitTextureSpec;
    }

    // Exact precedence at 0x001050E8..0x0010512C.
    if ((flags & 0x0Cu) == 0x0Cu) {
        return DefaultModelShader::LitBumpSpecNoTeamGround;
    }
    if ((flags & 0x244u) == 0x244u) {
        return DefaultModelShader::LitBumpSpecGlossGlowNoTeam;
    }
    if ((flags & 0x44u) == 0x44u) {
        return DefaultModelShader::LitBumpSpecGlossNoTeam;
    }
    if (flags & 0x4u) {
        return DefaultModelShader::LitBumpSpecNoTeam;
    }
    if (flags & 0x40u) {
        // The original helper deliberately exits without assigning a default
        // in this combination. Preserve that distinction.
        return DefaultModelShader::Unchanged;
    }
    return DefaultModelShader::LitBumpSpec;
}

std::string_view default_model_shader_name(DefaultModelShader shader) {
    switch (shader) {
    case DefaultModelShader::Unchanged:
        return {};
    case DefaultModelShader::LitObject:
        return "lit_object_shader";
    case DefaultModelShader::LitObjectNoTeamGround:
        return "lit_object_shader_no_team_ground";
    case DefaultModelShader::LitTexture:
        return "lit_texture_shader";
    case DefaultModelShader::LitBump:
        return "lit_bump_shader";
    case DefaultModelShader::LitTextureSpec:
        return "lit_texture_spec_shader";
    case DefaultModelShader::LitTextureSpecGloss:
        return "lit_texture_spec_gloss_shader";
    case DefaultModelShader::LitBumpSpec:
        return "lit_bump_spec_shader";
    case DefaultModelShader::LitBumpSpecNoTeam:
        return "lit_bump_spec_shader_no_team";
    case DefaultModelShader::LitBumpSpecNoTeamGround:
        return "lit_bump_spec_shader_no_team_ground";
    case DefaultModelShader::LitBumpSpecGlossNoTeam:
        return "lit_bump_spec_gloss_shader_no_team";
    case DefaultModelShader::LitBumpSpecGlossGlowNoTeam:
        return "lit_bump_spec_gloss_glow_shader_no_team";
    }
    return {};
}

}  // namespace comet::decomp
