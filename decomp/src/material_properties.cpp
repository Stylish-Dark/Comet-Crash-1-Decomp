#include "comet/material_properties.hpp"

#include <array>

namespace comet::decomp {
namespace {

constexpr std::array<MtlPropertyDirective, 4> kDirectives{{
    {"Ka", MtlPropertySemantic::AmbientColor,     0x00, 3},
    {"Kd", MtlPropertySemantic::DiffuseColor,     0x10, 3},
    {"Ks", MtlPropertySemantic::SpecularColor,    0x20, 3},
    {"Ns", MtlPropertySemantic::SpecularExponent, 0x30, 1},
}};

}  // namespace

const MtlPropertyDirective* classify_mtl_property_directive(
    std::string_view keyword) {
    for (const auto& directive : kDirectives) {
        if (directive.keyword == keyword) {
            return &directive;
        }
    }
    return nullptr;
}

}  // namespace comet::decomp
