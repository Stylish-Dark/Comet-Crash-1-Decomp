#include "comet/native_session.hpp"
#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
using namespace comet::decomp;
int main() {
    const auto folder = std::filesystem::temp_directory_path() / "comet-native-session-test";
    std::filesystem::create_directories(folder);
    std::array<unsigned char, kLevelMapHeaderSize + kLevelMapPrimaryRecordSize> blob{};
    blob[3] = 1;
    blob[kLevelMapHeaderSize] = kExtentRecordType;
    blob[kLevelMapHeaderSize + 2] = 12;
    blob[kLevelMapHeaderSize + 3] = 20;
    {
        std::ofstream output(folder / "level0.map", std::ios::binary);
        output.write(reinterpret_cast<const char*>(blob.data()), blob.size());
    }
    NativeSession session{};
    NativeSessionConfig config{};
    config.data_root = folder;
    assert(initialize_native_session(config, 0, session));
    assert(session.current_level_id == 0);
    assert(session.level.arena_extent == 20);
    assert(session.arena.extent == 20);
    assert(session.arena.players.size() == 4);
    assert(session.model_assets.size() == 37);
    assert(!session.targets.textures.empty());
    assert(change_native_level(1, session).error == NativeSessionError::MissingOrInvalidMap);
    assert(session.current_level_id == 0);
    auto invalid = config;
    invalid.arena.secondary_player = 4;
    assert(initialize_native_session(invalid, 0, session).error == NativeSessionError::InvalidArenaData);
    assert(session.arena.extent == 20);
    assert(session.level.arena_extent == 20);
    invalid = config;
    invalid.display_width = 0;
    assert(initialize_native_session(invalid, 0, session).error == NativeSessionError::InvalidDimensions);
    assert(session.current_level_id == 0);
    std::filesystem::remove(folder / "level0.map");
    std::filesystem::remove(folder);
}
