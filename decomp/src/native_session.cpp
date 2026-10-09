#include "comet/native_session.hpp"
#include <utility>

namespace comet::decomp {
namespace {
bool valid_dimensions(const NativeSessionConfig& config) {
    return config.display_width && config.display_height &&
           config.auxiliary_width && config.auxiliary_height;
}
}
NativeSessionResult initialize_native_session(
    const NativeSessionConfig& config, std::uint32_t level_id,
    NativeSession& session) {
    if (!valid_dimensions(config))
        return {NativeSessionError::InvalidDimensions, "zero render dimension"};
    LevelMapData new_level{};
    const auto path = level_map_path(config.data_root, level_id);
    if (!load_level_map_file(path, false, new_level))
        return {NativeSessionError::MissingOrInvalidMap,
                "could not parse " + path.string()};
    NativeSession next{};
    next.config = config;
    next.current_level_id = level_id;
    next.level = std::move(new_level);
    auto arena_config=config.arena;
    arena_config.level_id=level_id;
    const auto arena = initialize_arena_state(next.level, arena_config, next.arena);
    if (!arena) return {NativeSessionError::InvalidArenaData, arena.detail};
    next.targets = recover_arena_render_target_plan(
        config.display_width, config.display_height,
        config.auxiliary_width, config.auxiliary_height,
        config.renderer_mode);
    const auto assets = arena_model_asset_manifest();
    next.model_assets.assign(assets.begin(), assets.end());
    session = std::move(next);
    return {};
}
NativeSessionResult change_native_level(
    std::uint32_t level_id, NativeSession& session) {
    return initialize_native_session(session.config, level_id, session);
}
} // namespace comet::decomp
