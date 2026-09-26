#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace comet::decomp {

// Proven disk layout from PPU loader 0x000D91B0.
inline constexpr std::size_t kLevelMapHeaderSize = 0x88;
inline constexpr std::size_t kLevelMapPrimaryRecordSize = 0x38;
inline constexpr std::size_t kLevelMapSecondaryRecordSize = 0x18;
inline constexpr std::uint32_t kDefaultArenaExtent = 24;
inline constexpr std::uint8_t kExtentRecordType = 0x0B;

struct LevelMapHeader {
    std::array<std::uint8_t, kLevelMapHeaderSize> raw{};

    std::uint32_t primary_record_count() const;
    std::uint32_t secondary_record_count() const;
    std::uint32_t raw_u32(std::size_t index) const;
};

struct LevelMapPrimaryRecord {
    std::array<std::uint8_t, kLevelMapPrimaryRecordSize> raw{};

    std::uint8_t type() const { return raw[0]; }
};

struct LevelMapSecondaryRecord {
    std::array<std::uint8_t, kLevelMapSecondaryRecordSize> raw{};

    // Proven to be a big-endian float and monotonically nondecreasing in every
    // known secondary section. Exact gameplay meaning remains provisional.
    float order_value() const;
    std::uint8_t selector() const { return raw[6]; }
    std::uint8_t opcode() const { return raw[7]; }
};

struct LevelMapData {
    LevelMapHeader header{};
    std::vector<LevelMapPrimaryRecord> primary_records;
    std::vector<LevelMapSecondaryRecord> secondary_records;

    // PPU 0x000D91B0 writes the same value to runtime fields +0xA8/+0xAC.
    // Type-0x0B records provide byte extents; when absent the game uses 24.
    bool has_extent_record = false;
    std::uint32_t arena_extent = kDefaultArenaExtent;
};

// Exact filter implemented at PPU 0x000D947C..0x000D94B8.
bool keep_level_map_secondary_record(const LevelMapSecondaryRecord& record);

// Semantic replacement for the parsing half of PPU 0x000D91B0.
// skip_secondary mirrors that function's final byte/boolean argument:
// zero loads the conditional secondary section; nonzero omits it.
bool parse_level_map(
    const std::uint8_t* bytes,
    std::size_t size,
    bool skip_secondary,
    LevelMapData& out);

// Native file-backed path. For original level IDs 0..28, first run
// tools/extract_builtin_level_maps.py against the user's decrypted EBOOT so
// those formerly embedded blobs exist as ordinary level0.map..level28.map.
bool load_level_map_file(
    const std::filesystem::path& path,
    bool skip_secondary,
    LevelMapData& out);

std::filesystem::path level_map_path(
    const std::filesystem::path& data_root,
    std::uint32_t level_id);

}  // namespace comet::decomp
