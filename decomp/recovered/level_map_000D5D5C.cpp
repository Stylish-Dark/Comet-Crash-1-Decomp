// Recovered from NPEB00142 v1.00 PPU function 0x000D5D5C.
//
// This remains a transitional ABI-shaped rendition of the original call site.
// The callee at 0x000D91B0 is now structurally recovered; the native semantic
// parser lives in decomp/include/comet/level_map.hpp + decomp/src/level_map.cpp.

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace comet::recovered {

namespace {

constexpr std::size_t kCurrentLevelIdOffset = 0x2D451C;
constexpr std::size_t kLevelMapStateOffset = 0x2D6438;

// Constructor + transition cross-references now support the semantic name
// current_level_id for +0x2D451C.  See docs/decomp/root-state-map.md.

// The original code obtains this prefix from a global std::string-like object
// using its small-string/heap representation. The native rewrite should own
// this as a normal filesystem path.
const char* level_data_root();

// Original PPU loader 0x000D91B0..0x000DA254.
//
// Recovered semantics:
// - level IDs 0..28 read from the EBOOT's 29-entry embedded-map table;
// - higher IDs read the supplied path in "rb" mode;
// - parse 0x88 header + count0*0x38 primary + count1*0x18 secondary;
// - nonzero skip_secondary omits the conditional secondary-record section.
//
// The clean native replacement extracts IDs 0..28 once and then loads every
// level through the file-backed parser in comet::decomp.
int load_level_map_original_000D91B0(
    void* level_map_state,
    std::uint32_t level_id,
    const char* path,
    std::uint8_t skip_secondary);

template <typename T>
T& field(void* base, std::size_t offset) {
    return *reinterpret_cast<T*>(static_cast<std::uint8_t*>(base) + offset);
}

} // namespace

bool load_current_level_map_000D5D5C(
    void* game_state,
    std::uint8_t skip_secondary) {
    const std::uint32_t level_id =
        field<std::uint32_t>(game_state, kCurrentLevelIdOffset);

    char path[256]{};
    std::snprintf(
        path,
        sizeof(path),
        "%slevel%u.map",
        level_data_root(),
        level_id);

    void* level_map_state =
        static_cast<std::uint8_t*>(game_state) + kLevelMapStateOffset;

    return load_level_map_original_000D91B0(
               level_map_state,
               level_id,
               path,
               skip_secondary) != 0;
}

} // namespace comet::recovered
