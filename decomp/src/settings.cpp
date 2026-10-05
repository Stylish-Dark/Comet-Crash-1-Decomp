#include "comet/settings.hpp"

#include <fstream>

namespace comet::decomp {
namespace {

std::uint32_t read_be_u32(const std::uint8_t* p) {
    return
        (static_cast<std::uint32_t>(p[0]) << 24) |
        (static_cast<std::uint32_t>(p[1]) << 16) |
        (static_cast<std::uint32_t>(p[2]) << 8) |
        static_cast<std::uint32_t>(p[3]);
}

void write_be_u32(std::uint8_t* p, std::uint32_t value) {
    p[0] = static_cast<std::uint8_t>(value >> 24);
    p[1] = static_cast<std::uint8_t>(value >> 16);
    p[2] = static_cast<std::uint8_t>(value >> 8);
    p[3] = static_cast<std::uint8_t>(value);
}

}  // namespace

SettingsState default_settings() {
    // Matches the exact default-construction block at
    // 0x000D5E7C..0x000D5F44.
    return SettingsState{};
}

bool parse_settings(
    std::span<const std::uint8_t> bytes,
    SettingsState& out) {
    if (bytes.size() != kSettingsBlobSize ||
        bytes[0] != kSettingsFormatVersion) {
        return false;
    }

    SettingsState parsed{};
    parsed.header.format_version = bytes[0];
    parsed.header.field_01 = bytes[1];
    parsed.header.field_02 = bytes[2];
    parsed.header.field_03 = bytes[3];
    parsed.header.field_04 = read_be_u32(bytes.data() + 4);
    parsed.header.field_08 = bytes[8];
    parsed.header.field_09 = bytes[9];
    parsed.header.field_0A = bytes[10];
    parsed.header.field_0B = bytes[11];

    for (std::size_t bank = 0; bank < kSettingsBankCount; ++bank) {
        for (std::size_t lane = 0; lane < kSettingsLaneCount; ++lane) {
            for (std::size_t index = 0; index < kSettingsEntryCount; ++index) {
                parsed.values[bank][lane][index] = read_be_u32(
                    bytes.data() + settings_value_offset(bank, lane, index));
            }
        }
        for (std::size_t index = 0; index < kSettingsEntryCount; ++index) {
            parsed.flags[bank][index] =
                bytes[settings_flag_offset(bank, index)];
        }
    }

    out = parsed;
    return true;
}

std::array<std::uint8_t, kSettingsBlobSize> serialize_settings(
    const SettingsState& settings) {
    std::array<std::uint8_t, kSettingsBlobSize> bytes{};

    bytes[0] = settings.header.format_version;
    bytes[1] = settings.header.field_01;
    bytes[2] = settings.header.field_02;
    bytes[3] = settings.header.field_03;
    write_be_u32(bytes.data() + 4, settings.header.field_04);
    bytes[8] = settings.header.field_08;
    bytes[9] = settings.header.field_09;
    bytes[10] = settings.header.field_0A;
    bytes[11] = settings.header.field_0B;

    for (std::size_t bank = 0; bank < kSettingsBankCount; ++bank) {
        for (std::size_t lane = 0; lane < kSettingsLaneCount; ++lane) {
            for (std::size_t index = 0; index < kSettingsEntryCount; ++index) {
                write_be_u32(
                    bytes.data() + settings_value_offset(bank, lane, index),
                    settings.values[bank][lane][index]);
            }
        }
        for (std::size_t index = 0; index < kSettingsEntryCount; ++index) {
            bytes[settings_flag_offset(bank, index)] =
                settings.flags[bank][index];
        }
    }

    return bytes;
}

bool load_settings_file(
    const std::filesystem::path& path,
    SettingsState& out) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }
    const std::streamoff end = file.tellg();
    if (end != static_cast<std::streamoff>(kSettingsBlobSize)) {
        return false;
    }
    std::array<std::uint8_t, kSettingsBlobSize> bytes{};
    file.seekg(0, std::ios::beg);
    file.read(
        reinterpret_cast<char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
    return file && parse_settings(bytes, out);
}

bool save_settings_file(
    const std::filesystem::path& path,
    const SettingsState& settings) {
    const auto bytes = serialize_settings(settings);
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }
    file.write(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(file);
}

}  // namespace comet::decomp
