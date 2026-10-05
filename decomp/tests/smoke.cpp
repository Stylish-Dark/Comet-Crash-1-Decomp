#include "comet/arena_render_targets.hpp"
#include "comet/arena_assets.hpp"
#include "comet/level_map.hpp"
#include "comet/material_shader_policy.hpp"
#include "comet/material_programs.hpp"
#include "comet/material_textures.hpp"
#include "comet/material_properties.hpp"
#include "comet/model_geometry.hpp"
#include "comet/model_load_parameters.hpp"
#include "comet/model_obj_semantics.hpp"
#include "comet/model_submesh.hpp"
#include "comet/root_state.hpp"
#include "comet/model_batching.hpp"

#include <array>
#include <cassert>
#include <cstdint>

using namespace comet::decomp;

int main() {
    {
        static_assert(LegacyRootStateOffsets::arena_model_table == 0x2D2DC0);
        static_assert(LegacyRootStateOffsets::current_level_id == 0x2D451C);
        static_assert(LegacyRootStateOffsets::level_map_state == 0x2D6438);
        static_assert(kLegacyFramebufferHandleOffsets.size() == 11);
        static_assert(kLegacyTextureHandleOffsets.size() == 12);

        RootGameState root{};
        assert(root.current_level_id == 1);
        assert(root.game_mode == GameMode::Campaign);
        static_assert(static_cast<std::uint32_t>(GameMode::Training) == 1);
        static_assert(static_cast<std::uint32_t>(GameMode::Battle) == 2);
    }

    {
        constexpr auto batch =
            batched_stream_byte_counts(3, 100, 60);
        static_assert(batch.vertex_bytes == 9600);
        static_assert(batch.index_bytes == 720);
        static_assert(batch.object_info_bytes == 48);

        constexpr auto draw =
            batched_draw_counts(3, 100, 60);
        static_assert(draw.max_vertex == 299);
        static_assert(draw.index_count == 180);

        constexpr auto layout = batched_vertex_layout();
        static_assert(layout.stride == 0x20);
        static_assert(layout.primary_vec4_offset == 0x00);
        static_assert(layout.normal_ty_vec4_offset == 0x10);
    }

    {
        const ModelPosition raw{1.0f, 2.0f, -3.0f};
        const auto scaled = apply_geometry_scale(raw, 2.0f);
        assert(scaled.x == 2.0f);
        assert(scaled.y == 4.0f);
        assert(scaled.z == -6.0f);

        ModelLoadParameters transformed_params{};
        transformed_params.geometry_scale = 2.0f;
        transformed_params.geometry_y_offset = 5.0f;
        transformed_params.original_options = kModelOptionApplyGeometryYOffset;
        const auto transformed =
            transform_imported_position(raw, transformed_params);
        assert(transformed.x == 2.0f);
        assert(transformed.y == 9.0f);
        assert(transformed.z == -6.0f);

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

        MaterialProperties properties{};
        assert(properties.ambient.w == 0.0f);
        assert(properties.diffuse.w == 0.0f);
        assert(properties.specular.w == 0.0f);
        assert(!properties.use_team_color);
        assert(material_name_uses_team_color("team"));
        assert(material_name_uses_team_color("team.light"));
        assert(!material_name_uses_team_color("tesla"));

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
        ModelSubmesh submesh{};
        submesh.draw_range = {4, 6, 2, 9};
        submesh.material.properties.use_team_color = true;
        submesh.material.shader_base = "lit_texture_shader";
        submesh.material.legacy_program_paths =
            material_program_paths(submesh.material.shader_base);
        submesh.legacy_batched_path_available = true;
        assert(submesh.draw_range.index_byte_offset() == 8);
        assert(submesh.material.legacy_program_paths.standard_vertex ==
               "lit_texture_shader.vpo");
        assert(submesh.legacy_batched_path_available);
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
        const std::array<std::int32_t, 6> position_normal_tokens{
            1, 7, 2, 8, 3, 9,
        };
        const auto position_normal_face =
            decode_legacy_obj_face_tokens(position_normal_tokens);
        assert(position_normal_face.supported());
        assert(position_normal_face.reference_count == 3);
        assert(position_normal_face.references[0].position_index == 0);
        assert(position_normal_face.references[0].normal_index == 6);
        assert(position_normal_face.references[0].texcoord_index == -1);

        const std::array<std::int32_t, 12> quad_tokens{
            1, 11, 21,
            2, 12, 22,
            3, 13, 23,
            4, 14, 24,
        };
        const auto quad = decode_legacy_obj_face_tokens(quad_tokens);
        assert(quad.supported());
        assert(quad.reference_count == 6);
        assert(quad.references[0].position_index == 0);
        assert(quad.references[0].normal_index == 20);
        assert(quad.references[0].texcoord_index == 10);
        assert(quad.references[3].position_index == 2);
        assert(quad.references[4].position_index == 3);
        assert(quad.references[5].position_index == 0);

        static_assert(normalize_legacy_obj_index(1) == 0);
        static_assert(normalize_legacy_obj_index(-1) == -2);

        const std::array<ObjVec3, 1> positions{{{1.0f, 2.0f, 3.0f}}};
        const std::array<ObjVec3, 2> normals{{
            {0.0f, 1.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
        }};
        const std::array<ObjVec2, 1> texcoords{{{0.25f, 0.75f}}};
        const std::array<ObjSourceReference, 4> references{{
            {0, 0, 0},
            {0, 0, 0},
            {0, 1, 0},
            {0, 1, 0},
        }};
        const auto collapsed = collapse_legacy_obj_references(
            positions, normals, texcoords, references);
        assert(collapsed.valid);
        assert(collapsed.vertices.size() == 3);
        assert(collapsed.indices.size() == 4);
        assert(collapsed.indices[0] == 0);
        assert(collapsed.indices[1] == 0);
        assert(collapsed.indices[2] == 1);
        assert(collapsed.indices[3] == 2);
    }

    {
        const auto programs = material_program_paths("lit_texture_shader");
        assert(programs.standard_vertex == "lit_texture_shader.vpo");
        assert(programs.batched_vertex == "lit_texture_shader_spu.vpo");
        assert(programs.fragment == "lit_texture_shader.fpo");
        static_assert(
            LegacyMaterialProgramOffsets::standard_vertex_program == 0x48);
        static_assert(
            LegacyMaterialProgramOffsets::batched_vertex_program == 0x4C);
        static_assert(
            LegacyMaterialProgramOffsets::fragment_program == 0x50);
        static_assert(
            LegacyMaterialProgramOffsets::batch_enabled == 0x54);
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
