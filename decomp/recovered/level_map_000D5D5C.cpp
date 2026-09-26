// Recovered from NPEB00142 v1.00 PPU function 0x000D5D5C.
//
// This is intentionally transitional source: the control/data semantics have
// been recovered, while the root game-state type and callee at 0x000D91B0 are
// still unnamed. Keep the offsets visible until surrounding types are proven.

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace comet::recovered {

namespace {

constexpr std::size_t kCurrentLevelOffset = 0x2D451C;
constexpr std::size_t kLevelMapStateOffset = 0x2D6438;

// The original code obtains this prefix from a global std::string-like object
// using its small-string/heap representation. The native rewrite should own
// this as a normal path string; the accessor name is provisional.
const char* level_data_root();

// PPU 0x000D91B0. Signature reconstructed from the call site; return value is
// consumed as a boolean. Parameter names other than level/path remain unknown.
int sub_000D91B0(void* level_map_state,
                 std::uint32_t level_index,
                 const char* path,
                 std::uint8_t mode);

template <typename T>
T& field(void* base, std::size_t offset) {
    return *reinterpret_cast<T*>(static_cast<std::uint8_t*>(base) + offset);
}

} // namespace

bool load_level_map_000D5D5C(void* game_state, std::uint8_t mode) {
    const std::uint32_t level_index = field<std::uint32_t>(game_state, kCurrentLevelOffset);

    // Original stack frame reserves substantially more space, but this call site
    // formats only the level-map path. Keep a conservative fixed buffer until
    // the surrounding string/path type is recovered.
    char path[256]{};
    std::snprintf(path, sizeof(path), "%slevel%u.map", level_data_root(), level_index);

    void* level_map_state = static_cast<std::uint8_t*>(game_state) + kLevelMapStateOffset;
    return sub_000D91B0(level_map_state, level_index, path, mode) != 0;
}

} // namespace comet::recovered
