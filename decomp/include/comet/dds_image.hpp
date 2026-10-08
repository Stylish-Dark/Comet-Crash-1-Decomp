#pragma once
#include "comet/native_model.hpp"
#include <span>
namespace comet::decomp {
struct DdsImage { std::uint32_t width=0,height=0;std::vector<std::uint8_t> rgba; };
// Decode top mip only: masked RGB/luminance, DXT1/3/5. Cube/volume rejected.
AssetLoadResult decode_dds_image(std::span<const std::uint8_t> bytes,DdsImage& output);
AssetLoadResult load_dds_image(const std::filesystem::path& path,DdsImage& output);
}
