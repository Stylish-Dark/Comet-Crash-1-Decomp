#include "comet/arena_render_targets.hpp"
#include "comet/arena_assets.hpp"
#include "comet/level_map.hpp"
#include "comet/model_shader_policy.hpp"

#include <array>
#include <cassert>
#include <cstdint>

using namespace comet::decomp;

int main() {
    {
        ModelShaderInputs shader{};
        shader.has_diffuse = true;
        shader.has_specular = true;
        shader.has_bump = true;
        shader.original_options = 0x244;

        const auto selected = choose_default_model_shader(shader);
        assert(selected == DefaultModelShader::LitBumpSpecGlossGlowNoTeam);
        assert(default_model_shader_name(selected) ==
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
