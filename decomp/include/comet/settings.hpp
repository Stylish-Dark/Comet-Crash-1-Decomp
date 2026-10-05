#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace comet::decomp {

// Exact legacy persistence layout recovered from PPU 0x000D5BD8 and
// 0x000D5E18. settings.dat is written/read as one 3912-byte blob.
inline constexpr std::size_t kSettingsBlobSize = 3912;
inline constexpr std::uint8_t kSettingsFormatVersion = 8;
inline constexpr std::size_t kSettingsBankCount = 3;
inline constexpr std::size_t kSettingsLaneCount = 3;
inline constexpr std::size_t kSettingsEntryCount = 100;

struct SettingsHeader {
    std::uint8_t format_version = kSettingsFormatVersion;
    std::uint8_t field_01 = 1;
    std::uint8_t field_02 = 60;
    std::uint8_t field_03 = 90;
    std::uint32_t field_04 = 0;
    std::uint8_t field_08 = 1;
    std::uint8_t field_09 = 1;
    std::uint8_t field_0A = 0;
    std::uint8_t field_0B = 0;
};

// The original layout is structure-of-arrays rather than three contiguous
// records. Semantic names for the nine u32 lanes and three byte lanes are
// intentionally deferred until their gameplay/settings consumers are proven.
struct SettingsState {
    SettingsHeader header{};
    std::array<
        std::array<std::array<std::uint32_t, kSettingsEntryCount>, kSettingsLaneCount>,
        kSettingsBankCount> values{};
    std::array<std::array<std::uint8_t, kSettingsEntryCount>, kSettingsBankCount>
        flags{};
};

constexpr std::size_t settings_value_offset(
    std::size_t bank,
    std::size_t lane,
    std::size_t index) {
    return 12 + bank * 1200 + lane * 400 + index * 4;
}

constexpr std::size_t settings_flag_offset(
    std::size_t bank,
    std::size_t index) {
    return 3612 + bank * 100 + index;
}

SettingsState default_settings();

// Parses the exact big-endian PS3 settings.dat representation. The legacy
// loader rejects any size other than 3912 and any format-version byte != 8.
bool parse_settings(
    std::span<const std::uint8_t> bytes,
    SettingsState& out);

std::array<std::uint8_t, kSettingsBlobSize> serialize_settings(
    const SettingsState& settings);

bool load_settings_file(
    const std::filesystem::path& path,
    SettingsState& out);

bool save_settings_file(
    const std::filesystem::path& path,
    const SettingsState& settings);

}  // namespace comet::decomp
