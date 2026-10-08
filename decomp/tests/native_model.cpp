#include "comet/native_model.hpp"
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace comet::decomp;
void check(bool value) { if (!value) throw std::runtime_error("native model check failed"); }
int main() {
    NativeModel model;
    std::istringstream source("mtllib ship.mtl\nv 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nvn 0 0 1\nusemtl team.red\nf 1/1/1 2/2/1 3/3/1 4/4/1\nusemtl metal\nf -4 -2 -1\n");
    check(bool(parse_obj_model(source, {2,-1}, 0x20, 3, model)));
    check(model.indices.size() == 9 && model.vertices.size() == 7);
    check(model.submeshes.size() == 2 && model.submeshes[1].range.first_index == 6);
    check(model.vertices[2].position.x == 2 && model.vertices[2].position.y == 5);
    check(model.vertices[4].normal.z == 1);
    check(model.material_libraries[0] == "ship.mtl");
    check(model.signed_bounding_radius < -5);
    const auto old_count = model.indices.size();
    for (const char* bad : {"v nan 0 0\n", "v 0 0\n", "v 0 0 0\nf 1 2 3\n", "v 0 0 0\n"}) {
        std::istringstream invalid(bad);
        check(!parse_obj_model(invalid, {}, 0, 0, model));
        check(model.indices.size() == old_count);
    }
    std::vector<NativeMaterial> materials;
    std::istringstream mtl("newmtl team.red\nKa 0.1 0.2 0.3\nKd 0.8 0.4 0.2\nKs 1 1 1\nNs 50\nmap_Kd colors.dds\nnewmtl metal\nKd 0.2 0.3 0.4\n");
    check(bool(parse_mtl_materials(mtl, materials)));
    check(materials.size() == 2 && materials[0].properties.use_team_color);
    check(std::abs(materials[0].properties.scaled_specular_exponent - 6.4f) < 0.0001f);
    check(materials[0].diffuse_texture == "colors.dds");
    std::istringstream texture_options("newmtl stone\nbump normal.tif -bm 1\n");
    std::vector<NativeMaterial> option_materials;
    check(bool(parse_mtl_materials(texture_options,option_materials)));
    check(option_materials[0].bump_texture == "normal.tif");
    std::istringstream bad_mtl("newmtl bad\nKd inf 0 1\n");
    check(!parse_mtl_materials(bad_mtl, materials) && materials.size() == 2);
    std::cout << "native OBJ/MTL model checks passed\n";
}
