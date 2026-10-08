#pragma once
#include "comet/asset_store.hpp"
namespace comet::decomp {
struct NativeViewerConfig {
    int width=1280,height=720;
    std::uint32_t frame_limit=0; // zero runs until quit
    std::uint32_t initial_model=0;
    std::filesystem::path screenshot;
    bool hidden=false;
};
AssetLoadResult run_native_viewer(const std::vector<ArenaNativeModel>& models,
    const NativeViewerConfig& config);
} // namespace comet::decomp
