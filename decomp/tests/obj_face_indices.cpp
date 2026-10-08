#include "comet/obj_face_indices.hpp"

#include <cassert>
#include <cstdint>
#include <limits>
#include <vector>

using namespace comet::decomp;

int main() {
    const ObjSourceCounts counts{4, 3, 2};
    const auto full = parse_obj_face_vertex("1/2/2", counts);
    assert(full && full.vertex.position == 0);
    assert(full.vertex.texcoord == 1 && full.vertex.normal == 1);
    const auto signed_positive = parse_obj_face_vertex("+1/+2/+2", counts);
    assert(signed_positive && signed_positive.vertex == full.vertex);
    assert(parse_obj_face_vertex("+", counts).error == ObjReferenceError::Syntax);
    const auto back = parse_obj_face_vertex("-1/-2/-1", counts);
    assert(back && back.vertex.position == 3);
    assert(back.vertex.texcoord == 1 && back.vertex.normal == 1);
    const auto bare = parse_obj_face_vertex("2", counts);
    assert(bare && bare.vertex.position == 1);
    assert(!bare.vertex.texcoord && !bare.vertex.normal);
    const auto without_uv = parse_obj_face_vertex("3//1", counts);
    assert(without_uv && without_uv.vertex.position == 2);
    assert(!without_uv.vertex.texcoord && without_uv.vertex.normal == 0);
    const auto with_uv = parse_obj_face_vertex("4/3", counts);
    assert(with_uv && with_uv.vertex.texcoord == 2 && !with_uv.vertex.normal);

    assert(parse_obj_face_vertex("0/1/1", counts).error == ObjReferenceError::ZeroIndex);
    assert(parse_obj_face_vertex("-5", counts).error == ObjReferenceError::OutOfRange);
    assert(parse_obj_face_vertex("5", counts).error == ObjReferenceError::OutOfRange);
    assert(parse_obj_face_vertex("1/4/1", counts).error == ObjReferenceError::OutOfRange);
    assert(parse_obj_face_vertex("1//3", counts).error == ObjReferenceError::OutOfRange);
    assert(parse_obj_face_vertex("1/", counts).error == ObjReferenceError::Syntax);
    assert(parse_obj_face_vertex("1//", counts).error == ObjReferenceError::Syntax);
    assert(parse_obj_face_vertex("1/2/3/4", counts).error == ObjReferenceError::Syntax);
    assert(parse_obj_face_vertex("1garbage", counts).error == ObjReferenceError::Syntax);
    assert(parse_obj_face_vertex("999999999999999999999", counts).error == ObjReferenceError::Syntax);

    const auto dedup = deduplicate_obj_face_vertices({
        full.vertex, bare.vertex, full.vertex, without_uv.vertex, bare.vertex
    });
    assert(!dedup.index_overflow);
    assert(dedup.unique_vertices.size() == 3);
    assert((dedup.indices == std::vector<std::uint16_t>{0, 1, 0, 2, 1}));

    std::vector<ObjFaceVertex> many;
    many.reserve(65537);
    for (std::size_t i = 0; i < 65537; ++i) many.push_back({i, {}, {}});
    const auto overflow = deduplicate_obj_face_vertices(many);
    assert(overflow.index_overflow && overflow.indices.empty());
}
