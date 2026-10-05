#include "comet/model_obj_semantics.hpp"

#include <cmath>
#include <limits>

namespace comet::decomp {
namespace {

bool component_mismatch(const float lhs, const float rhs) {
    return std::fabs(lhs - rhs) > kLegacyObjAttributeEpsilon;
}

bool normal_is_unclaimed(const ObjVec3& normal) {
    return std::fabs(normal.x) <= kLegacyObjAttributeEpsilon &&
           std::fabs(normal.y) <= kLegacyObjAttributeEpsilon &&
           std::fabs(normal.z) <= kLegacyObjAttributeEpsilon;
}

bool normal_mismatch(const ObjVec3& lhs, const ObjVec3& rhs) {
    return component_mismatch(lhs.x, rhs.x) ||
           component_mismatch(lhs.y, rhs.y) ||
           component_mismatch(lhs.z, rhs.z);
}

bool texcoord_mismatch(const ObjVec2& lhs, const ObjVec2& rhs) {
    return component_mismatch(lhs.x, rhs.x) ||
           component_mismatch(lhs.y, rhs.y);
}

ObjSourceReference decode_full_corner(
    const std::span<const std::int32_t> tokens,
    const std::size_t offset) {
    // OBJ text is v/vt/vn. The 12-byte staging record is v/vn/vt.
    return {
        normalize_legacy_obj_index(tokens[offset + 0]),
        normalize_legacy_obj_index(tokens[offset + 2]),
        normalize_legacy_obj_index(tokens[offset + 1]),
    };
}

bool valid_nonnegative_index(
    const std::int32_t index,
    const std::size_t size) {
    return index >= 0 &&
           static_cast<std::size_t>(index) < size;
}

}  // namespace

DecodedObjFace decode_legacy_obj_face_tokens(
    const std::span<const std::int32_t> component_tokens) {
    DecodedObjFace face{};

    if (component_tokens.size() ==
        kLegacyObjTrianglePositionNormalTokenCount) {
        for (std::size_t corner = 0; corner < 3; ++corner) {
            const std::size_t offset = corner * 2;
            face.references[corner] = {
                normalize_legacy_obj_index(component_tokens[offset + 0]),
                normalize_legacy_obj_index(component_tokens[offset + 1]),
                kLegacyObjMissingTexcoord,
            };
        }
        face.reference_count = 3;
        return face;
    }

    if (component_tokens.size() == kLegacyObjTriangleFullTokenCount) {
        for (std::size_t corner = 0; corner < 3; ++corner) {
            face.references[corner] =
                decode_full_corner(component_tokens, corner * 3);
        }
        face.reference_count = 3;
        return face;
    }

    if (component_tokens.size() == kLegacyObjQuadFullTokenCount) {
        std::array<ObjSourceReference, 4> quad{};
        for (std::size_t corner = 0; corner < quad.size(); ++corner) {
            quad[corner] =
                decode_full_corner(component_tokens, corner * 3);
        }

        // The second triangle is emitted in the original cyclic order 2,3,0.
        face.references[0] = quad[0];
        face.references[1] = quad[1];
        face.references[2] = quad[2];
        face.references[3] = quad[2];
        face.references[4] = quad[3];
        face.references[5] = quad[0];
        face.reference_count = 6;
        return face;
    }

    return face;
}

ObjReferenceCollapseResult collapse_legacy_obj_references(
    const std::span<const ObjVec3> positions,
    const std::span<const ObjVec3> normals,
    const std::span<const ObjVec2> texcoords,
    const std::span<const ObjSourceReference> references,
    const ObjReferenceCollapseOptions options) {
    ObjReferenceCollapseResult result{};
    result.vertices.reserve(positions.size() + references.size());
    result.indices.reserve(references.size());

    for (const auto& position : positions) {
        result.vertices.push_back({position, {}, {}});
    }

    for (const auto& reference : references) {
        if (!valid_nonnegative_index(
                reference.position_index, positions.size()) ||
            !valid_nonnegative_index(
                reference.normal_index, normals.size())) {
            return result;
        }

        const bool has_texcoord =
            reference.texcoord_index != kLegacyObjMissingTexcoord;
        if (has_texcoord &&
            !valid_nonnegative_index(
                reference.texcoord_index, texcoords.size())) {
            return result;
        }

        auto& base = result.vertices[
            static_cast<std::size_t>(reference.position_index)];
        const auto& source_normal =
            normals[static_cast<std::size_t>(reference.normal_index)];

        std::uint32_t final_index =
            static_cast<std::uint32_t>(reference.position_index);

        if (normal_is_unclaimed(base.normal)) {
            base.normal = source_normal;
            if (has_texcoord) {
                base.texcoord =
                    texcoords[static_cast<std::size_t>(
                        reference.texcoord_index)];
            }
        } else {
            bool needs_split = false;
            if (has_texcoord) {
                const auto& source_texcoord =
                    texcoords[static_cast<std::size_t>(
                        reference.texcoord_index)];
                if (options.split_on_normal_mismatch &&
                    normal_mismatch(base.normal, source_normal)) {
                    needs_split = true;
                } else if (
                    texcoord_mismatch(base.texcoord, source_texcoord)) {
                    needs_split = true;
                }

                if (needs_split) {
                    final_index =
                        static_cast<std::uint32_t>(result.vertices.size());
                    result.vertices.push_back(
                        {base.position, source_normal, source_texcoord});
                }
            } else if (normal_mismatch(base.normal, source_normal)) {
                needs_split = true;
                final_index =
                    static_cast<std::uint32_t>(result.vertices.size());
                result.vertices.push_back(
                    {base.position, source_normal, {}});
            }

            (void)needs_split;
        }

        // The original final store is a halfword write. Title assets stay far
        // below this range; reject overflow in the native rewrite instead of
        // silently reproducing wraparound.
        if (final_index >
            static_cast<std::uint32_t>(
                std::numeric_limits<std::uint16_t>::max())) {
            return result;
        }
        result.indices.push_back(static_cast<std::uint16_t>(final_index));
    }

    result.valid = true;
    return result;
}

}  // namespace comet::decomp
