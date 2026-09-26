#include "comet/arena_render_targets.hpp"
#include "comet/arena_assets.hpp"
#include "comet/level_map.hpp"
#include "comet/material_shader_policy.hpp"
#include "comet/material_textures.hpp"
#include "comet/material_properties.hpp"
#include "comet/model_geometry.hpp"
#include "comet/model_load_parameters.hpp"

#include <array>
#include <cassert>
#include <cstdint>

using namespace comet::decomp;

int main() {
    {
        const ModelPosition raw{1.0f, 2.0f, -3.0f};
        const auto scaled = apply_geometry_scale(raw, 2.0f);
        assert(scaled.x == 2.0f);
        assert(scaled.y == 4.0f);
        assert(scaled.z == -6.0f);

        const std::array<ModelPosition, 2> positions{{
            {3.0f, 4.0f, 0.0f},
            {0.0f, 0.0f, 2.0f},
        }};
        const float positive =
            compute_signed_bounding_radius(positions, 0.5f);
        const float negative =
            compute_signed_bounding_radius(positions, -0.5f);
        assert(positive > 2.49f && positive < 2.51f);
        assert(negative < -2.49f && negative > -2.51f);
    }

    {
        const auto* kd = classify_mtl_property_directive("Kd");
        assert(kd != nullptr);
        assert(kd->semantic == MtlPropertySemantic::DiffuseColor);
        assert(kd->legacy_material_offset == 0x10);
        assert(kd->component_count == 3);

        const auto* ns = classify_mtl_property_directive("Ns");
        assert(ns != nullptr);
        assert(ns->semantic == MtlPropertySemantic::SpecularExponent);
        assert(ns->legacy_material_offset == 0x30);
        assert(scale_mtl_specular_exponent(100.0f) > 12.79f);
        assert(scale_mtl_specular_exponent(100.0f) < 12.81f);
    }

    {
        const auto* diffuse = classify_mtl_texture_directive("map_Kd");
        assert(diffuse != nullptr);
        assert(diffuse->semantic == MaterialTextureSemantic::Diffuse);
        assert(diffuse->legacy_material_offset == 0x38);

        const auto* cube = classify_mtl_texture_directive("cube");
        assert(cube != nullptr);
        assert(cube->semantic == MaterialTextureSemantic::EnvironmentCube);
        assert(cube->loader_kind == MaterialTextureLoaderKind::CubeTexture);
        assert(cube->legacy_material_offset == 0x44);
    }

    {
        const auto full = full_vertex_layout();
        assert(full.stride == 0x20);
        assert(full.attribute_count == 3);
        assert(full.attributes[1].semantic == VertexSemantic::Normal);
        assert(full.attributes[2].byte_offset == 0x18);

        SubmeshDrawRange range{};
        range.first_index = 7;
        range.index_count = 12;
        range.min_vertex = 3;
        range.max_vertex = 19;
        assert(range.index_byte_offset() == 14);

        const auto compact = compact_vertex_layout();
        assert(compact.stride == 0x14);
        assert(compact.attribute_count == 2);
        assert(compact.attributes[1].semantic == VertexSemantic::TexCoord0);
        assert(compact.attributes[1].byte_offset == 0x0C);
    }

    {
        MaterialShaderInputs shader{};
        shader.has_diffuse = true;
        shader.has_specular = true;
        shader.has_bump = true;
        shader.original_options = 0x244;

        const auto selected = choose_default_material_shader(shader);
        assert(selected == DefaultMaterialShader::LitBumpSpecGlossGlowNoTeam);
        assert(default_material_shader_name(selected) ==
               "lit_bump_spec_gloss_glow_shader_no_team");
    }

    {
        const auto assets = arena_model_asset_manifest();
        assert(assets.size() == 37);
        assert(assets.front().original_root_offset == 0x2D2DC0);
        assert(assets.front().original_slot_index == 0);
        assert(assets.back().original_root_offset == 0x2D4170);
    }

    {
        const auto plan =
            recover_arena_render_target_plan(720, 480, 480, 256, 2);

        assert(plan.display_scale_x == 2);
        assert(plan.display_scale_y == 1);
        assert(plan.legacy_parameter_6022 == 0x6031);
        assert(!plan.textures.empty());
        assert(!plan.attachments.empty());
        assert(plan.small_target_preflight.storage == TextureStorage::Rgba16Float);
    }

    {
        std::array<std::uint8_t,
            kLevelMapHeaderSize +
            kLevelMapPrimaryRecordSize +
            kLevelMapSecondaryRecordSize> bytes{};

        bytes[3] = 1;
        bytes[7] = 1;

        const std::size_t p = kLevelMapHeaderSize;
        bytes[p + 0] = kExtentRecordType;
        bytes[p + 2] = 16;
        bytes[p + 3] = 20;

        const std::size_t s = p + kLevelMapPrimaryRecordSize;
        bytes[s + 6] = 1;
        bytes[s + 7] = 21;

        LevelMapData map{};
        assert(parse_level_map(bytes.data(), bytes.size(), false, map));
        assert(map.has_extent_record);
        assert(map.arena_extent == 20);
        assert(map.primary_records.size() == 1);
        assert(map.primary_records[0].raw[2] == 20);
        assert(map.primary_records[0].raw[3] == 20);
        assert(map.secondary_records.size() == 1);
    }

    return 0;
}
