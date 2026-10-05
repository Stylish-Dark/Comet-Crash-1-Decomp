#pragma once

#include <array>
#include <cstdint>

namespace comet::decomp {

// Proven offsets inside the original monolithic arena/root object. These are
// reversing provenance only: the native port should own normal typed members
// rather than reproduce a 0x2Dxxxx-byte PS3 object layout.
struct LegacyRootStateOffsets {
    static constexpr std::uint32_t player_object_table = 0x214920;
    static constexpr std::uint32_t arena_model_table = 0x2D2DC0;
    static constexpr std::uint32_t current_level_id = 0x2D451C;
    static constexpr std::uint32_t provisional_transition_level_id = 0x2D4520;
    static constexpr std::uint32_t player_count = 0x2D4538;
    static constexpr std::uint32_t game_mode = 0x2D4560;
    static constexpr std::uint32_t settings_state = 0x2D4598;
    static constexpr std::uint32_t level_map_state = 0x2D6438;
};

inline constexpr std::uint32_t kLegacyArenaModelStride = 0x90;
inline constexpr std::uint32_t kLegacyArenaModelSlotCount = 40;
inline constexpr std::uint32_t kLegacyPlayerSlotCount = 4;

// Resource-handle slots recovered from arenaGraphics.cpp. Native renderer
// resources replace these integer PSGL handles; the offsets remain useful for
// correlating renderer functions back to the original executable.
inline constexpr std::array<std::uint32_t, 11> kLegacyFramebufferHandleOffsets{{
    0x2D445C, 0x2D4460, 0x2D4468, 0x2D446C, 0x2D4470, 0x2D4474,
    0x2D4478, 0x2D447C, 0x2D4480, 0x2D4484, 0x2D4488,
}};

inline constexpr std::array<std::uint32_t, 12> kLegacyTextureHandleOffsets{{
    0x2D448C, 0x2D4490, 0x2D4494, 0x2D4498, 0x2D449C, 0x2D44A0,
    0x2D44A4, 0x2D44A8, 0x2D44AC, 0x2D44B0, 0x2D44B4, 0x2D44B8,
}};

enum class GameMode : std::uint32_t {
    Campaign = 0,
    Training = 1,
    Battle = 2,

    // Value 3 is compared explicitly by original gameplay/UI code, but its
    // source-level meaning has not yet been proved.
    Unknown3 = 3,
};

struct RootGameState {
    // Native semantic state already strong enough to promote out of raw offsets.
    std::uint32_t current_level_id = 1;

    // Kept provisional because original code often mirrors it into
    // current_level_id during transitions, but its exact requested/next/display
    // distinction is not yet independently proven.
    std::uint32_t transition_level_id = 0;

    std::uint32_t player_count = 0;
    GameMode game_mode = GameMode::Campaign;
};

}  // namespace comet::decomp
