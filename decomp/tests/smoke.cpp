#include "comet/arena_render_targets.hpp"
#include "comet/level_map.hpp"

#include <array>
#include <cassert>
#include <cstdint>

using namespace comet::decomp;

int main() {
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
