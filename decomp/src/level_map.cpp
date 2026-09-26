#include "comet/level_map.hpp"

#include <cstring>
#include <fstream>
#include <limits>
#include <string>

namespace comet::decomp {
namespace {

std::uint32_t read_be_u32(const std::uint8_t* p) {
    return
        (static_cast<std::uint32_t>(p[0]) << 24) |
        (static_cast<std::uint32_t>(p[1]) << 16) |
        (static_cast<std::uint32_t>(p[2]) << 8) |
        static_cast<std::uint32_t>(p[3]);
}

float read_be_f32(const std::uint8_t* p) {
    const std::uint32_t bits = read_be_u32(p);
    float value = 0.0f;
    static_assert(sizeof(value) == sizeof(bits));
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

bool checked_add_mul(
    std::size_t base,
    std::uint32_t count,
    std::size_t stride,
    std::size_t& out) {
    if (count > (std::numeric_limits<std::size_t>::max() - base) / stride) {
        return false;
    }
    out = base + static_cast<std::size_t>(count) * stride;
    return true;
}

}  // namespace

std::uint32_t LevelMapHeader::primary_record_count() const {
    return read_be_u32(raw.data() + 0x00);
}

std::uint32_t LevelMapHeader::secondary_record_count() const {
    return read_be_u32(raw.data() + 0x04);
}

std::uint32_t LevelMapHeader::raw_u32(std::size_t index) const {
    if (index >= raw.size() / 4) {
        return 0;
    }
    return read_be_u32(raw.data() + index * 4);
}

float LevelMapSecondaryRecord::order_value() const {
    return read_be_f32(raw.data());
}

bool keep_level_map_secondary_record(const LevelMapSecondaryRecord& record) {
    const std::uint8_t selector = record.selector();
    const std::uint8_t opcode = record.opcode();

    if (selector > 10) {
        return false;
    }

    if (selector == 9 || selector == 10) {
        return opcode != 29;
    }

    return opcode >= 20 && opcode <= 30 && opcode != 29;
}

bool parse_level_map(
    const std::uint8_t* bytes,
    std::size_t size,
    bool skip_secondary,
    LevelMapData& out) {
    if (bytes == nullptr || size < kLevelMapHeaderSize) {
        return false;
    }

    LevelMapData parsed{};
    std::memcpy(parsed.header.raw.data(), bytes, kLevelMapHeaderSize);

    std::size_t after_primary = 0;
    if (!checked_add_mul(
            kLevelMapHeaderSize,
            parsed.header.primary_record_count(),
            kLevelMapPrimaryRecordSize,
            after_primary)) {
        return false;
    }

    std::size_t expected_size = 0;
    if (!checked_add_mul(
            after_primary,
            parsed.header.secondary_record_count(),
            kLevelMapSecondaryRecordSize,
            expected_size)) {
        return false;
    }

    if (size != expected_size) {
        return false;
    }

    parsed.primary_records.reserve(parsed.header.primary_record_count());

    std::size_t pos = kLevelMapHeaderSize;
    for (std::uint32_t i = 0; i < parsed.header.primary_record_count(); ++i) {
        LevelMapPrimaryRecord record{};
        std::memcpy(record.raw.data(), bytes + pos, record.raw.size());
        pos += record.raw.size();

        if (record.type() == kExtentRecordType) {
            parsed.has_extent_record = true;
            parsed.arena_extent =
                record.raw[2] > record.raw[3] ? record.raw[2] : record.raw[3];

            // Exact mutation performed by the original loader before storing
            // the primary record.
            record.raw[2] = static_cast<std::uint8_t>(parsed.arena_extent);
            record.raw[3] = static_cast<std::uint8_t>(parsed.arena_extent);
        }

        parsed.primary_records.push_back(record);
    }

    if (parsed.has_extent_record && !skip_secondary) {
        parsed.secondary_records.reserve(parsed.header.secondary_record_count());

        for (std::uint32_t i = 0; i < parsed.header.secondary_record_count(); ++i) {
            LevelMapSecondaryRecord record{};
            std::memcpy(record.raw.data(), bytes + pos, record.raw.size());
            pos += record.raw.size();

            if (keep_level_map_secondary_record(record)) {
                parsed.secondary_records.push_back(record);
            }
        }
    }

    out = std::move(parsed);
    return true;
}

bool load_level_map_file(
    const std::filesystem::path& path,
    bool skip_secondary,
    LevelMapData& out) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }

    const std::streamoff end = file.tellg();
    if (end < 0) {
        return false;
    }

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
    file.seekg(0, std::ios::beg);

    if (!bytes.empty()) {
        file.read(
            reinterpret_cast<char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
        if (!file) {
            return false;
        }
    }

    return parse_level_map(bytes.data(), bytes.size(), skip_secondary, out);
}

std::filesystem::path level_map_path(
    const std::filesystem::path& data_root,
    std::uint32_t level_id) {
    return data_root / ("level" + std::to_string(level_id) + ".map");
}

}  // namespace comet::decomp
