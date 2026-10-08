#include "comet/native_session.hpp"
#include <charconv>
#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: comet_native <extracted-level-map-directory> [level-id]\n";
        return 2;
    }
    std::uint32_t level_id = 0;
    if (argc == 3) {
        const std::string_view arg(argv[2]);
        const auto parsed = std::from_chars(arg.data(), arg.data() + arg.size(), level_id);
        if (parsed.ec != std::errc{} || parsed.ptr != arg.data() + arg.size()) {
            std::cerr << "Invalid level ID\n";
            return 2;
        }
    }
    comet::decomp::NativeSessionConfig config{};
    config.data_root = argv[1];
    comet::decomp::NativeSession session{};
    const auto result = comet::decomp::initialize_native_session(config, level_id, session);
    if (!result) {
        std::cerr << result.detail << '\n';
        return 1;
    }
    std::cout << "Comet Crash native arena bootstrap\n"
              << "Level: " << session.current_level_id << '\n'
              << "Extent: " << session.level.arena_extent << '\n'
              << "Primary records: " << session.level.primary_records.size() << '\n'
              << "Secondary records: " << session.level.secondary_records.size() << '\n'
              << "Model asset entries: " << session.model_assets.size() << '\n'
              << "Render targets: " << session.targets.textures.size() << '\n';
    return 0;
}
