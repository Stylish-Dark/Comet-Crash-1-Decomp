#pragma once
#include "comet/arena_assets.hpp"
#include "comet/native_model.hpp"
#include <optional>
namespace comet::decomp {
class AssetStore {
public:
    explicit AssetStore(std::filesystem::path root);
    const std::filesystem::path& root() const { return root_; }
    // Relative game paths only. Canonicalization also prevents symlink escape.
    std::optional<std::filesystem::path> resolve(const std::filesystem::path& relative) const;
    AssetLoadResult load_model(const std::filesystem::path& relative,
        ModelLoadParameters parameters, std::uint32_t options,
        float vertex_y_offset, NativeModel& output) const;
private:
    std::filesystem::path root_;
};
struct ArenaNativeModel { ArenaModelAssetSpec spec; NativeModel model; };
AssetLoadResult load_arena_models(const AssetStore& assets, std::vector<ArenaNativeModel>& output);
} // namespace comet::decomp
