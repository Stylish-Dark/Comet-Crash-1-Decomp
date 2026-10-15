#include "comet/obj_geometry.hpp"
#include <charconv>
#include <limits>
#include <string>
#include <unordered_map>

namespace comet::decomp {
namespace {
std::optional<std::size_t> index_component(std::string_view token,
                                            std::size_t count) {
    if (token.empty()) return std::nullopt;
    std::int64_t signed_index = 0;
    const auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), signed_index);
    if (ec != std::errc{} || ptr != token.data() + token.size() ||
        signed_index == 0) return std::nullopt;
    if (signed_index > 0) {
        const auto positive = static_cast<std::uint64_t>(signed_index);
        if (positive > count) return std::nullopt;
        return static_cast<std::size_t>(positive - 1);
    }
    // Avoid converting an arbitrary size_t count into a signed integer.
    const auto distance = static_cast<std::uint64_t>(-(signed_index + 1)) + 1;
    if (distance > count) return std::nullopt;
    return count - static_cast<std::size_t>(distance);
}
struct KeyHash {
    std::size_t operator()(const ObjVertexKey& k) const {
        std::size_t h = std::hash<std::size_t>{}(k.position);
        auto mix = [&](std::size_t v) {
            h ^= v + std::size_t{0x9e3779b9} + (h << 6) + (h >> 2);
        };
        mix(k.texcoord ? *k.texcoord + 1 : 0);
        mix(k.normal ? *k.normal + 1 : 0);
        return h;
    }
};
} // namespace

std::optional<ObjVertexKey> parse_obj_vertex_reference(
    std::string_view reference, ObjSourceCounts counts) {
    const auto slash1 = reference.find('/');
    const auto position = index_component(reference.substr(0, slash1), counts.positions);
    if (!position) return std::nullopt;
    ObjVertexKey result{};
    result.position = *position;
    if (slash1 == std::string_view::npos) return result;
    const auto slash2 = reference.find('/', slash1 + 1);
    const auto texture = reference.substr(slash1 + 1,
        slash2 == std::string_view::npos ? slash2 : slash2 - slash1 - 1);
    if (!texture.empty()) {
        result.texcoord = index_component(texture, counts.texcoords);
        if (!result.texcoord) return std::nullopt;
    } else if (slash2 == std::string_view::npos) {
        return std::nullopt; // p/ is not a valid face reference
    }
    if (slash2 != std::string_view::npos) {
        const auto normal = reference.substr(slash2 + 1);
        if (normal.empty() || normal.find('/') != std::string_view::npos)
            return std::nullopt;
        result.normal = index_component(normal, counts.normals);
        if (!result.normal) return std::nullopt;
    }
    return result;
}

std::optional<ObjIndexedFaces> index_obj_polygons(
    const std::vector<std::vector<std::string_view>>& polygons,
    ObjSourceCounts counts) {
    ObjIndexedFaces result{};
    std::unordered_map<ObjVertexKey, std::uint16_t, KeyHash> dedup;
    for (const auto& polygon : polygons) {
        if (polygon.size() < 3) return std::nullopt;
        std::vector<std::uint16_t> polygon_indices;
        for (const auto& token : polygon) {
            auto key = parse_obj_vertex_reference(token, counts);
            if (!key) return std::nullopt;
            const auto it = dedup.find(*key);
            if (it != dedup.end()) {
                polygon_indices.push_back(it->second);
                continue;
            }
            // u16 indices permit indices 0..65535 (65536 unique vertices).
            if (result.unique_vertices.size() > std::numeric_limits<std::uint16_t>::max())
                return std::nullopt;
            const auto idx = static_cast<std::uint16_t>(result.unique_vertices.size());
            result.unique_vertices.push_back(*key);
            dedup.emplace(*key, idx);
            polygon_indices.push_back(idx);
        }
        for (std::size_t i = 1; i + 1 < polygon_indices.size(); ++i) {
            result.indices.push_back(polygon_indices[0]);
            result.indices.push_back(polygon_indices[i]);
            result.indices.push_back(polygon_indices[i + 1]);
        }
    }
    return result;
}
} // namespace comet::decomp
