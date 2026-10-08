#include "comet/obj_polygon_mesh.hpp"
#include <cassert>
#include <vector>
using namespace comet::decomp;
int main() {
    ObjTriangleMeshBuilder mesh;
    const ObjFaceVertex a{0,{},{}}, b{1,{},{}}, c{2,{},{}}, d{3,{},{}};
    const std::vector<std::vector<ObjFaceVertex>> first{{a,b,c,d}};
    assert(mesh.append_submesh(first));
    assert((mesh.indices() == std::vector<std::uint16_t>{0,1,2,0,2,3}));
    assert(mesh.vertices().size() == 4);
    assert(mesh.ranges().size() == 1);
    assert(mesh.ranges()[0].first_index == 0);
    assert(mesh.ranges()[0].index_count == 6);
    assert(mesh.ranges()[0].min_vertex == 0);
    assert(mesh.ranges()[0].max_vertex == 3);
    const std::vector<std::vector<ObjFaceVertex>> second{{d,c,b}};
    assert(mesh.append_submesh(second));
    assert(mesh.vertices().size() == 4);
    assert(mesh.ranges()[1].first_index == 6);
    assert(mesh.ranges()[1].index_count == 3);
    assert(mesh.ranges()[1].min_vertex == 1);
    assert(mesh.ranges()[1].max_vertex == 3);
    const auto old_indices = mesh.indices();
    const auto old_vertices = mesh.vertices();
    const auto old_ranges = mesh.ranges().size();
    const std::vector<std::vector<ObjFaceVertex>> invalid{{a,b}};
    assert(mesh.append_submesh(invalid).error == ObjPolygonError::TooFewVertices);
    assert(mesh.indices() == old_indices);
    assert(mesh.vertices() == old_vertices);
    assert(mesh.ranges().size() == old_ranges);
    // All 65,536 addressable u16 vertices are legal; a 65,537th is not.
    ObjTriangleMeshBuilder boundary;
    std::vector<ObjFaceVertex> refs;
    refs.reserve(65536);
    for (std::size_t i = 0; i < 65536; ++i)
        refs.push_back({i, {}, {}});
    const std::vector<std::vector<ObjFaceVertex>> limit{refs};
    assert(boundary.append_submesh(limit));
    assert(boundary.vertices().size() == 65536);
    assert(boundary.indices().size() == (65536 - 2) * 3);
    const auto boundary_indices = boundary.indices().size();
    const std::vector<std::vector<ObjFaceVertex>> over{{{65536, {}, {}},
                                                           {0, {}, {}},
                                                           {1, {}, {}}}};
    assert(boundary.append_submesh(over).error == ObjPolygonError::IndexOverflow);
    assert(boundary.indices().size() == boundary_indices);
    assert(boundary.vertices().size() == 65536);
    return 0;
}
