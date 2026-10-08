#include "comet/obj_face_indices.hpp"

#include <charconv>
#include <limits>
#include <unordered_map>

namespace comet::decomp {
namespace {
struct VertexHash {
    std::size_t operator()(const ObjFaceVertex& v) const {
        std::size_t h = std::hash<std::size_t>{}(v.position);
        const auto mix = [&h](std::size_t x) {
            h ^= x + std::size_t{0x9e3779b9} + (h << 6) + (h >> 2);
        };
        mix(v.texcoord ? *v.texcoord : std::numeric_limits<std::size_t>::max());
        mix(v.normal ? *v.normal : std::numeric_limits<std::size_t>::max());
        mix(v.texcoord.has_value());
        mix(v.normal.has_value());
        return h;
    }
};

std::optional<std::size_t> resolve(std::string_view token, std::size_t count,
                                   ObjReferenceError& error) {
    if (token.empty()) { error = ObjReferenceError::Syntax; return std::nullopt; }
    std::int64_t raw = 0;
    const char* first = token.data();
    const char* last = first + token.size();
    const auto parsed = std::from_chars(first, last, raw);
    if (parsed.ec != std::errc{} || parsed.ptr != last) {
        error = ObjReferenceError::Syntax;
        return std::nullopt;
    }
    if (raw == 0) { error = ObjReferenceError::ZeroIndex; return std::nullopt; }
    // Avoid narrowing huge source-array counts into signed arithmetic.
    if (raw > 0) {
        const auto one_based = static_cast<std::uint64_t>(raw);
        if (one_based > count) { error = ObjReferenceError::OutOfRange; return std::nullopt; }
        return static_cast<std::size_t>(one_based - 1);
    }
    const auto distance = static_cast<std::uint64_t>(-(raw + 1)) + 1;
    if (distance > count) { error = ObjReferenceError::OutOfRange; return std::nullopt; }
    return count - static_cast<std::size_t>(distance);
}
} // namespace

ObjReferenceResult parse_obj_face_vertex(std::string_view token, ObjSourceCounts counts) {
    ObjReferenceResult result;
    const auto a = token.find('/');
    const auto b = a == std::string_view::npos ? a : token.find('/', a + 1);
    if (b != std::string_view::npos && token.find('/', b + 1) != std::string_view::npos) {
        result.error = ObjReferenceError::Syntax;
        return result;
    }
    const auto pos = resolve(token.substr(0, a), counts.positions, result.error);
    if (!pos) return result;
    result.vertex.position = *pos;
    if (a == std::string_view::npos) return result;

    const auto tex = token.substr(a + 1, b == std::string_view::npos
                                  ? b : b - a - 1);
    if (!tex.empty()) {
        result.vertex.texcoord = resolve(tex, counts.texcoords, result.error);
        if (!result.vertex.texcoord) return result;
    }
    if (b == std::string_view::npos) {
        if (tex.empty()) result.error = ObjReferenceError::Syntax;
        return result;
    }
    const auto norm = token.substr(b + 1);
    if (norm.empty()) { result.error = ObjReferenceError::Syntax; return result; }
    result.vertex.normal = resolve(norm, counts.normals, result.error);
    return result;
}

ObjIndexBuffer deduplicate_obj_face_vertices(const std::vector<ObjFaceVertex>& faces) {
    ObjIndexBuffer output;
    std::unordered_map<ObjFaceVertex, std::uint16_t, VertexHash> seen;
    output.indices.reserve(faces.size());
    for (const auto& face : faces) {
        auto it = seen.find(face);
        if (it == seen.end()) {
            if (output.unique_vertices.size() > std::numeric_limits<std::uint16_t>::max()) {
                output.index_overflow = true;
                output.indices.clear();
                return output;
            }
            const auto index = static_cast<std::uint16_t>(output.unique_vertices.size());
            output.unique_vertices.push_back(face);
            it = seen.emplace(face, index).first;
        }
        output.indices.push_back(it->second);
    }
    return output;
}
} // namespace comet::decomp
