#pragma once

#include <cstdint>
#include <vector>

namespace comet::decomp {

enum class TextureStorage {
    Rgba8,
    Rgb16Float,
    Rgba16Float,
    Depth32,
};

enum class TextureFilter {
    Nearest,
    Linear,
};

enum class TextureWrap {
    Clamp,
};

enum class TargetAttachment {
    None,
    Color0,
    Color1,
    Depth,
};

enum class TargetSizeSource {
    Display,
    ScaledDisplay,
    Auxiliary,
    Fixed384x384,
    Fixed80x64,
};

struct RecoveredTextureSpec {
    // Root-game-state offsets are provenance for the original handle slots;
    // the native renderer must use its own resource objects.
    std::uint32_t original_texture_offset = 0;
    TargetSizeSource size_source = TargetSizeSource::Display;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    TextureStorage storage = TextureStorage::Rgba8;
    TextureFilter min_filter = TextureFilter::Nearest;
    TextureFilter mag_filter = TextureFilter::Nearest;
    TextureWrap wrap_s = TextureWrap::Clamp;
    TextureWrap wrap_t = TextureWrap::Clamp;
};

struct RecoveredAttachmentSpec {
    std::uint32_t original_framebuffer_offset = 0;
    std::uint32_t original_texture_offset = 0;
    TargetAttachment attachment = TargetAttachment::None;
};

struct ArenaRenderTargetPlan {
    std::uint32_t display_width = 0;
    std::uint32_t display_height = 0;
    std::uint32_t auxiliary_width = 0;
    std::uint32_t auxiliary_height = 0;
    std::uint32_t display_scale_x = 1;
    std::uint32_t display_scale_y = 1;

    // PSGL 0x6022 value selected by the original mode. Retained strictly as
    // reversing provenance; native backends should not depend on it.
    std::uint32_t legacy_parameter_6022 = 0;

    std::vector<RecoveredTextureSpec> textures;
    std::vector<RecoveredAttachmentSpec> attachments;

    // Original 0xE5A34 briefly creates +44A8 as RGBA16F before redefining it
    // as RGBA8 in the small MRT loop. Preserve that evidence separately.
    RecoveredTextureSpec small_target_preflight{};
};

ArenaRenderTargetPlan recover_arena_render_target_plan(
    std::uint32_t display_width,
    std::uint32_t display_height,
    std::uint32_t auxiliary_width,
    std::uint32_t auxiliary_height,
    std::uint32_t renderer_mode);

}  // namespace comet::decomp
