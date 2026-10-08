#include "comet/asset_store.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>
using namespace comet::decomp;
void check(bool x) { if (!x) throw std::runtime_error("asset store check failed"); }
int main() {
    const auto root=std::filesystem::temp_directory_path()/"comet-asset-store-test";
    std::filesystem::create_directories(root/"models");
    { std::ofstream f(root/"models/a.obj");f<<"mtllib a.mtl\nv 0 0 0\nv 1 0 0\nv 0 1 0\nusemtl team\nf 1 2 3\n"; }
    { std::ofstream f(root/"models/a.mtl");f<<"newmtl team\nKd 0.2 0.4 0.6\nmap_Kd color.dds\n"; }
    AssetStore assets(root); NativeModel model;
    { std::ofstream f(root/"models/a.mtl");f<<"newmtl team\nKd 0.2 0.4 0.6\nmap_Kd /export/machine/color.tif\n"; }
    check(bool(assets.load_model("models/a.obj",{},0,0,model)));
    check(model.submeshes[0].material.properties.diffuse.b==0.6f);
    check(model.submeshes[0].material.diffuse_texture==std::filesystem::weakly_canonical(root/"models/color.dds"));
    check(!assets.resolve("../escape.obj"));check(!assets.resolve(root/"models/a.obj"));
    check(!assets.load_model("missing.obj",{},0,0,model));check(model.indices.size()==3);
    { std::ofstream f(root/"models/a.mtl");f<<"newmtl team\nmap_Kd ../../outside.dds\n"; }
    check(!assets.load_model("models/a.obj",{},0,0,model));check(model.indices.size()==3);
    std::filesystem::remove_all(root);
}
