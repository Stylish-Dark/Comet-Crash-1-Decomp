#pragma once
#include "comet/arena_assets.hpp"
#include "comet/arena_state.hpp"
#include "comet/arena_render_targets.hpp"
#include "comet/level_map.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace comet::decomp {

// Native-owned application state. It deliberately has no guest-memory layout.
struct NativeSessionConfig {
    std::filesystem::path data_root;
    std::uint32_t display_width = 1280;
    std::uint32_t display_height = 720;
    std::uint32_t auxiliary_width = 640;
    std::uint32_t auxiliary_height = 360;
    std::uint32_t renderer_mode = 0;
    ArenaLoadConfig arena;
};
struct NativeSession {
    NativeSessionConfig config;
    std::uint32_t current_level_id = 0;
    LevelMapData level;
    ArenaState arena;
    ArenaRenderTargetPlan targets;
    std::vector<ArenaModelAssetSpec> model_assets;
};
enum class NativeSessionError { None, InvalidDimensions, MissingOrInvalidMap, InvalidArenaData };
struct NativeSessionResult {
    NativeSessionError error = NativeSessionError::None;
    std::string detail;
    explicit operator bool() const { return error == NativeSessionError::None; }
};

// Transactional load: retains the old session on failure.
NativeSessionResult initialize_native_session(
    const NativeSessionConfig& config, std::uint32_t level_id,
    NativeSession& session);
NativeSessionResult change_native_level(std::uint32_t level_id, NativeSession& session);
} // namespace comet::decomp
