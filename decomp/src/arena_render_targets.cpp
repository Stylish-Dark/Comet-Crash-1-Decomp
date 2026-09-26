#include "comet/arena_render_targets.hpp"

namespace comet::decomp {
namespace {

RecoveredTextureSpec texture(
    std::uint32_t offset,
    TargetSizeSource source,
    std::uint32_t width,
    std::uint32_t height,
    TextureStorage storage,
    TextureFilter min_filter = TextureFilter::Nearest,
    TextureFilter mag_filter = TextureFilter::Nearest) {
    RecoveredTextureSpec spec{};
    spec.original_texture_offset = offset;
    spec.size_source = source;
    spec.width = width;
    spec.height = height;
    spec.storage = storage;
    spec.min_filter = min_filter;
    spec.mag_filter = mag_filter;
    return spec;
}

void attach(
    ArenaRenderTargetPlan& plan,
    std::uint32_t framebuffer_offset,
    std::uint32_t texture_offset,
    TargetAttachment attachment) {
    plan.attachments.push_back({
        framebuffer_offset,
        texture_offset,
        attachment,
    });
}

}  // namespace

ArenaRenderTargetPlan recover_arena_render_target_plan(
    std::uint32_t display_width,
    std::uint32_t display_height,
    std::uint32_t auxiliary_width,
    std::uint32_t auxiliary_height,
    std::uint32_t renderer_mode) {
    ArenaRenderTargetPlan plan{};
    plan.display_width = display_width;
    plan.display_height = display_height;
    plan.auxiliary_width = auxiliary_width;
    plan.auxiliary_height = auxiliary_height;

    // Exact branch policy at 0x000E5A90 / 0x000E65A4..0x000E65D0.
    switch (renderer_mode) {
    case 0:
        plan.display_scale_x = 2;
        plan.display_scale_y = 2;
        plan.legacy_parameter_6022 = 0x6033;
        break;
    case 1:
        plan.display_scale_x = 2;
        plan.display_scale_y = 2;
        plan.legacy_parameter_6022 = 0x6032;
        break;
    case 2:
        plan.display_scale_x = 2;
        plan.display_scale_y = 1;
        plan.legacy_parameter_6022 = 0x6031;
        break;
    default:
        plan.display_scale_x = 1;
        plan.display_scale_y = 1;
        plan.legacy_parameter_6022 = 0x6030;
        break;
    }

    const std::uint32_t scaled_width =
        display_width * plan.display_scale_x;
    const std::uint32_t scaled_height =
        display_height * plan.display_scale_y;

    // 0xE5ABC onward.
    plan.textures.push_back(texture(
        0x4490, TargetSizeSource::Display,
        display_width, display_height, TextureStorage::Rgba8));

    plan.textures.push_back(texture(
        0x4498, TargetSizeSource::ScaledDisplay,
        scaled_width, scaled_height, TextureStorage::Rgba8));
    attach(plan, 0x445C, 0x4498, TargetAttachment::Color0);

    plan.textures.push_back(texture(
        0x448C, TargetSizeSource::ScaledDisplay,
        scaled_width, scaled_height, TextureStorage::Depth32));
    attach(plan, 0x445C, 0x448C, TargetAttachment::Depth);

    // 0xE5D68..0xE5E40.
    plan.textures.push_back(texture(
        0x449C, TargetSizeSource::Auxiliary,
        auxiliary_width, auxiliary_height, TextureStorage::Rgb16Float));
    attach(plan, 0x4468, 0x449C, TargetAttachment::Color0);

    // 0xE5E44..0xE5F28.
    plan.textures.push_back(texture(
        0x44B8, TargetSizeSource::Fixed384x384,
        384, 384, TextureStorage::Rgba8));
    attach(plan, 0x4488, 0x44B8, TargetAttachment::Color0);

    // 0xE5F2C..0xE6000 and 0xE6004..0xE60D4.
    plan.textures.push_back(texture(
        0x44A0, TargetSizeSource::Auxiliary,
        auxiliary_width, auxiliary_height, TextureStorage::Rgb16Float));
    attach(plan, 0x446C, 0x44A0, TargetAttachment::Color0);

    plan.textures.push_back(texture(
        0x44A4, TargetSizeSource::Auxiliary,
        auxiliary_width, auxiliary_height, TextureStorage::Rgb16Float));
    attach(plan, 0x4470, 0x44A4, TargetAttachment::Color0);

    // Two stand-alone small color targets.
    plan.textures.push_back(texture(
        0x44AC, TargetSizeSource::Fixed80x64,
        80, 64, TextureStorage::Rgba8));
    attach(plan, 0x4480, 0x44AC, TargetAttachment::Color0);

    plan.textures.push_back(texture(
        0x44B0, TargetSizeSource::Fixed80x64,
        80, 64, TextureStorage::Rgba8));
    attach(plan, 0x4484, 0x44B0, TargetAttachment::Color0);

    // One-time preflight/allocation observed before the common three-FBO loop.
    plan.small_target_preflight = texture(
        0x44A8, TargetSizeSource::Fixed80x64,
        80, 64, TextureStorage::Rgba16Float);
    attach(plan, 0x4474, 0x44A8, TargetAttachment::Color0);

    // Final 80x64 MRT layout. +44AC/+44B0 are intentionally reused from
    // the stand-alone FBOs above. +44B4 is shared as COLOR1 by all three.
    plan.textures.push_back(texture(
        0x44A8, TargetSizeSource::Fixed80x64,
        80, 64, TextureStorage::Rgba8));
    plan.textures.push_back(texture(
        0x44B4, TargetSizeSource::Fixed80x64,
        80, 64, TextureStorage::Rgba8));

    attach(plan, 0x4474, 0x44A8, TargetAttachment::Color0);
    attach(plan, 0x4474, 0x44B4, TargetAttachment::Color1);
    attach(plan, 0x4478, 0x44AC, TargetAttachment::Color0);
    attach(plan, 0x4478, 0x44B4, TargetAttachment::Color1);
    attach(plan, 0x447C, 0x44B0, TargetAttachment::Color0);
    attach(plan, 0x447C, 0x44B4, TargetAttachment::Color1);

    return plan;
}

}  // namespace comet::decomp
