#include "comet/material_programs.hpp"

namespace comet::decomp {
namespace {

std::string append_suffix(
    const std::string_view base,
    const std::string_view suffix) {
    std::string path;
    path.reserve(base.size() + suffix.size());
    path.append(base);
    path.append(suffix);
    return path;
}

}  // namespace

MaterialProgramPaths material_program_paths(const std::string_view shader_base) {
    return {
        append_suffix(shader_base, kLegacyVertexProgramSuffix),
        append_suffix(shader_base, kLegacyBatchedVertexProgramSuffix),
        append_suffix(shader_base, kLegacyFragmentProgramSuffix),
    };
}

}  // namespace comet::decomp
