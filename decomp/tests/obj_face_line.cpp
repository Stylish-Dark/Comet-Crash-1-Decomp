#include "comet/obj_face_line.hpp"
#include "comet/obj_polygon_mesh.hpp"

#include <cassert>
#include <cstdint>
#include <vector>

using namespace comet::decomp;

int main() {
    const ObjSourceCounts counts{5, 4, 3};
    const auto triangle = parse_obj_face_line("  f 1/1/1 -1/-1/-1 3//2 # triangle", counts);
    assert(triangle);
    assert(triangle.vertices.size() == 3);
    assert(triangle.vertices[0].position == 0);
    assert(triangle.vertices[1].position == 4);
    assert(triangle.vertices[1].texcoord == 3);
    assert(triangle.vertices[1].normal == 2);
    assert(!triangle.vertices[2].texcoord);
    assert(triangle.vertices[2].normal == 1);

    const auto quad = parse_obj_face_line("f\t1 2 3 4", counts);
    assert(quad && quad.vertices.size() == 4);
    assert(parse_obj_face_line("v 1 2 3", counts).error ==
           ObjFaceLineError::NotFaceDirective);
    assert(parse_obj_face_line("f 1 2", counts).error ==
           ObjFaceLineError::TooFewVertices);
    const auto bad = parse_obj_face_line("f 1 2 99", counts);
    assert(bad.error == ObjFaceLineError::InvalidReference);
    assert(bad.reference_error == ObjReferenceError::OutOfRange);
    assert(bad.vertices.empty());
    const auto zero = parse_obj_face_line("f 1 2 0", counts);
    assert(zero.reference_error == ObjReferenceError::ZeroIndex);
    assert(parse_obj_face_line("f# empty", counts).error ==
           ObjFaceLineError::TooFewVertices);

    ObjTriangleMeshBuilder mesh;
    std::vector<std::vector<ObjFaceVertex>> faces{
        triangle.vertices, quad.vertices
    };
    assert(mesh.append_submesh(faces));
    assert(mesh.indices().size() == 9);
    assert(mesh.ranges().size() == 1);
    assert(mesh.ranges()[0].index_count == 9);

    // An invalid subsequent submesh must not partially add vertices.
    const auto vertex_count = mesh.vertices().size();
    const auto index_count = mesh.indices().size();
    const std::vector<std::vector<ObjFaceVertex>> invalid{{{100, {}, {}}}};
    assert(mesh.append_submesh(invalid).error == ObjPolygonError::TooFewVertices);
    assert(mesh.vertices().size() == vertex_count);
    assert(mesh.indices().size() == index_count);
}
