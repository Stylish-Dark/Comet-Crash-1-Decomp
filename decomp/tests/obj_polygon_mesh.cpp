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
    return 0;
}
