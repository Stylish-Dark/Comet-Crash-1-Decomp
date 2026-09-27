#pragma once

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace comet::decomp {

// The original material binder uploads Ka/Kd/Ks through vec4 Cg parameters.
// MTL parsing writes only RGB; the fourth lane is zero-initialized when the
// 0x78-byte submesh record is created and remains zero on the recovered path.
struct MaterialColor4 {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float w = 0.0f;
};

// Proven scalar/color fields in the legacy material subobject at submesh+0x10.
// Offsets are relative to that material object.
struct LegacyMaterialPropertyOffsets {
    static constexpr std::uint32_t ambient_r = 0x00;
    static constexpr std::uint32_t ambient_g = 0x04;
    static constexpr std::uint32_t ambient_b = 0x08;
    static constexpr std::uint32_t ambient_w = 0x0C;

    static constexpr std::uint32_t diffuse_r = 0x10;
    static constexpr std::uint32_t diffuse_g = 0x14;
    static constexpr std::uint32_t diffuse_b = 0x18;
    static constexpr std::uint32_t diffuse_w = 0x1C;

    static constexpr std::uint32_t specular_r = 0x20;
    static constexpr std::uint32_t specular_g = 0x24;
    static constexpr std::uint32_t specular_b = 0x28;
    static constexpr std::uint32_t specular_w = 0x2C;

    static constexpr std::uint32_t scaled_specular_exponent = 0x30;
    static constexpr std::uint32_t use_team_color = 0x34;
};

// Exact multiplier applied by the original MTL Ns path at PPU 0x0010A658.
inline constexpr float kLegacyMtlNsScale = 0.12800000607967377f;

struct MaterialProperties {
    MaterialColor4 ambient{};
    MaterialColor4 diffuse{};
    MaterialColor4 specular{};
    float scaled_specular_exponent = 0.0f;
    bool use_team_color = false;
};

enum class MtlPropertySemantic {
    AmbientColor,
    DiffuseColor,
    SpecularColor,
    SpecularExponent,
};

struct MtlPropertyDirective {
    std::string_view keyword;
    MtlPropertySemantic semantic{};
    std::uint32_t legacy_material_offset = 0;
    std::uint32_t component_count = 0;
};

const MtlPropertyDirective* classify_mtl_property_directive(
    std::string_view keyword);

// The shipped code stores Ns * 0.128000006... in material+0x30.
constexpr float scale_mtl_specular_exponent(float ns) {
    return ns * kLegacyMtlNsScale;
}

// Exact recovered "team" prefix policy from the newmtl path. The original
// comparison uses at most four characters. All shipped team-colored material
// names are "team" or "team.*".
constexpr bool material_name_uses_team_color(std::string_view name) {
    constexpr std::string_view prefix = "team";
    const std::size_t compared = std::min(name.size(), prefix.size());
    return compared != 0 &&
        name.substr(0, compared) == prefix.substr(0, compared);
}

}  // namespace comet::decomp
