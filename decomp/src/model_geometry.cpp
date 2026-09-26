#include "comet/model_geometry.hpp"

namespace comet::decomp {

RecoveredVertexLayout full_vertex_layout() {
    RecoveredVertexLayout layout{};
    layout.stride = static_cast<std::uint32_t>(kLegacyFullVertexStride);
    layout.attributes[0] = {VertexSemantic::Position, 0x00, 3};
    layout.attributes[1] = {VertexSemantic::Normal, 0x0C, 3};
    layout.attributes[2] = {VertexSemantic::TexCoord0, 0x18, 2};
    layout.attribute_count = 3;
    return layout;
}

RecoveredVertexLayout compact_vertex_layout() {
    RecoveredVertexLayout layout{};
    layout.stride = static_cast<std::uint32_t>(kLegacyCompactVertexStride);
    layout.attributes[0] = {VertexSemantic::Position, 0x00, 3};
    layout.attributes[1] = {VertexSemantic::TexCoord0, 0x0C, 2};
    layout.attribute_count = 2;
    return layout;
}

}  // namespace comet::decomp
