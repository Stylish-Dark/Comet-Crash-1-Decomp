#include "comet/obj_geometry.hpp"
#include <cassert>
#include <string_view>
#include <vector>
using namespace comet::decomp;
int main() {
    const ObjSourceCounts counts{4, 4, 2};
    const auto a = parse_obj_vertex_reference("1/2/1", counts);
    const auto b = parse_obj_vertex_reference("-4/-3/-2", counts);
    assert(a && b && *a == *b);
    assert(a->position == 0 && *a->texcoord == 1 && *a->normal == 0);
    assert(parse_obj_vertex_reference("1//-1", counts));
    assert(parse_obj_vertex_reference("1", counts));
    assert(!parse_obj_vertex_reference("0/2/1", counts));
    assert(!parse_obj_vertex_reference("5/2/1", counts));
    assert(!parse_obj_vertex_reference("-5/2/1", counts));
    assert(!parse_obj_vertex_reference("1/5/1", counts));
    assert(!parse_obj_vertex_reference("1/2/3", counts));
    assert(!parse_obj_vertex_reference("1/", counts));
    assert(!parse_obj_vertex_reference("1//", counts));
    assert(!parse_obj_vertex_reference("1/2/1/1", counts));
    assert(!parse_obj_vertex_reference("junk", counts));
    const std::vector<std::vector<std::string_view>> polygons{
        {"1/1/1", "2/2/1", "3/3/1", "4/4/1"},
        {"-4/-4/-2", "-2/-2/-2", "-1/-1/-2"}
    };
    const auto mesh = index_obj_polygons(polygons, counts);
    assert(mesh && mesh->unique_vertices.size() == 4);
    assert((mesh->indices == std::vector<std::uint16_t>{0,1,2,0,2,3,0,2,3}));
    assert(!index_obj_polygons({{"1","2","5"}}, counts));
    assert(!index_obj_polygons({{"1","2"}}, counts));
}
